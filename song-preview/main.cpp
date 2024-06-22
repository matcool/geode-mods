#include "Geode/ui/TextInput.hpp"
#include <Geode/Geode.hpp>
#include <unordered_map>
#include <UIBuilder.hpp>
#include <span>

using namespace geode::prelude;

struct SampleInfo {
	std::vector<float> samples;
	using SampleTask = Task<std::vector<float>>;

	static std::pair<Task<std::vector<float>>, float> fromFile(std::string_view songPath, int targetWidth) {
		auto* engine = FMODAudioEngine::sharedEngine();

		FMOD::Sound* sound;
		const FMOD_MODE mode = FMOD_DEFAULT | FMOD_OPENONLY;
		auto result = engine->m_system->createSound(songPath.data(), mode, nullptr, &sound);
		log::info("path is {}", songPath.data());
		if (result != FMOD_OK) {
			log::info("Error creating sound! {}", int(result));
		}
		unsigned int lengthMilliseconds;
		result = sound->getLength(&lengthMilliseconds, FMOD_TIMEUNIT_MS);
		if (result != FMOD_OK) {
			log::info("Error getting length {}", int(result));
		}
		float songLength = static_cast<float>(lengthMilliseconds) / 1000.f;

		return std::make_pair(SampleTask::run([sound, targetWidth](auto, auto hasBeenCancelled) -> SampleTask::Result {
			FMOD_SOUND_FORMAT format;
			int channels;
			sound->getFormat(nullptr, &format, &channels, nullptr);

			if (format != FMOD_SOUND_FORMAT_PCM8 && format != FMOD_SOUND_FORMAT_PCM16 && format != FMOD_SOUND_FORMAT_PCM24 &&
				format != FMOD_SOUND_FORMAT_PCM32 && format != FMOD_SOUND_FORMAT_PCMFLOAT) {
				log::error("Invalid format! {}", int(format));
				return SampleTask::Cancel();
			}

			unsigned int rawDataSize;
			unsigned int sampleCount;
			auto result = sound->getLength(&rawDataSize, FMOD_TIMEUNIT_PCMBYTES);
			if (result != FMOD_OK) {
				log::error("Error reading size! {}", int(result));
			}
			sound->getLength(&sampleCount, FMOD_TIMEUNIT_PCM);
			unsigned int bytesPerSample = rawDataSize / sampleCount;
			auto rawData = std::make_unique<uint8_t[]>(rawDataSize);
			unsigned int readCount;
			result = sound->readData(rawData.get(), rawDataSize, &readCount);
			if (result != FMOD_OK) {
				// ignore EOF here because what it reads is good enough, lol
				// maybe im calculating the size wrong,,, but blame config
				
				// log::error("Error reading data! {}, only read {} out of {}", int(result), readCount, rawDataSize);
				// return SampleTask::Cancel();
			}

			sound->release();

			sampleCount = rawDataSize / bytesPerSample;
			bytesPerSample /= channels;
			const auto getSampleAt = [&](int sampleIndex) {
				if (sampleIndex < 0 || sampleIndex >= sampleCount)
					return 0.f;
				float sample = 0.f;
				size_t dataIndex = sampleIndex * bytesPerSample * channels;
				if (dataIndex > rawDataSize) {
					// shh
					return 0.f;
				}
				for (int j = 0; j < channels; j++) {
					uint32_t rawChannelSample;
					switch (format) {
						case FMOD_SOUND_FORMAT_PCM8:
							rawChannelSample = rawData[dataIndex + 0];
							sample += std::bit_cast<int8_t>(static_cast<uint8_t>(rawChannelSample)) / 128.f;
							break;
						case FMOD_SOUND_FORMAT_PCM16:
							rawChannelSample = (rawData[dataIndex + 0] << 0 |
												rawData[dataIndex + 1] << 8);
							sample += std::bit_cast<int16_t>(static_cast<uint16_t>(rawChannelSample)) / (128.f * 255.f);
							break;
						case FMOD_SOUND_FORMAT_PCM24:
							// TODO: this is wrong, doesnt properly read sign bit
							rawChannelSample = (rawData[dataIndex + 0] << 0 |
												rawData[dataIndex + 1] << 8 |
												rawData[dataIndex + 2] << 16);
							sample += rawChannelSample / (128.f * 255.f * 255.f);
							break;
						case FMOD_SOUND_FORMAT_PCM32:
							rawChannelSample = (rawData[dataIndex + 0] << 0  |
												rawData[dataIndex + 1] << 8  |
												rawData[dataIndex + 2] << 16 |
												rawData[dataIndex + 3] << 24);
							sample += std::bit_cast<int32_t>(rawChannelSample) / (128.f * 255.f * 255.f * 255.f);
							break;
						case FMOD_SOUND_FORMAT_PCMFLOAT:
							rawChannelSample = (rawData[dataIndex + 0] << 0  |
												rawData[dataIndex + 1] << 8  |
												rawData[dataIndex + 2] << 16 |
												rawData[dataIndex + 3] << 24);
							sample += std::bit_cast<float>(rawChannelSample);
							break;
						default:
							break;
					}
					dataIndex += bytesPerSample;
				}
				sample /= channels;
				return sample;
			};

			std::vector<float> samples;
			samples.reserve(targetWidth);

			for (float x = 0.f; x < targetWidth; x += 1.f) {
				if (hasBeenCancelled()) {
					return SampleTask::Cancel();
				}
				int index = static_cast<int>(x / targetWidth * sampleCount);
				float sample = 0.f;
				// how many samples to look at per bar we render
				int total = float(sampleCount) / targetWidth * 0.5f;
				for (int i = -total/2; i < total/2; i++) {
					sample = std::max(sample, std::abs(getSampleAt(index + i)));
				}
				samples.push_back(sample);
			}

			return samples;
		}), songLength);
	}
};

std::string formatSeconds(int seconds) {
	return fmt::format("{:02}:{:02}", seconds / 60, seconds % 60);
}

std::pair<CCPoint, bool> testTouchInside(CCTouch* touch, CCNode* node) {
	auto location = touch->getLocation();
	auto nodeLocation = node->convertToNodeSpace(location);
	auto size = node->getContentSize();
	bool inside = nodeLocation.x >= 0.f && nodeLocation.x <= size.width &&
		nodeLocation.y >= 0.f && nodeLocation.y <= size.height;
	return {nodeLocation, inside};
}

class DetailedAudioPreviewPopup : public geode::Popup<int, CustomSongWidget*> {
protected:
	int m_songId;
	CCNode* m_drawNode;
	CCNode* m_overlayNode;
	CCNode* m_overlay2Node;
	CCLabelBMFont* m_timeLabel;
	float m_songLength;
	float m_startOffset = 0.f;
	geode::TextInput* m_startOffsetInput;
	CCSprite* m_cursorNode;
	CCSize m_widgetSize;
	CCSize m_barSize;
	CustomSongWidget* m_parentWidget = nullptr;

	EventListener<SampleInfo::SampleTask> m_sampleTaskListener;

    bool setup(int value, CustomSongWidget* parent) override {
		this->setID("DetailedAudioPreviewPopup"_spr);

		m_parentWidget = parent;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
		m_songId = value;

		auto path = MusicDownloadManager::sharedState()->pathForSong(m_songId);
		path = CCFileUtils::get()->fullPathForFilename(path.c_str(), false);
		
		FMODAudioEngine::sharedEngine()->stopAllMusic();

		float targetWidth = winSize.width / 2.f;
		auto frameSize = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize();

		float pixel = winSize.width / frameSize.width;
		float barWidth = pixel * 1.f;
		float barHeight = 50.f;
		m_widgetSize = ccp(targetWidth, barHeight);
		m_barSize = ccp(barWidth, barHeight);

		auto info = SampleInfo::fromFile(path, targetWidth / pixel);
		m_songLength = info.second;
		m_sampleTaskListener.bind([this](SampleInfo::SampleTask::Event* event) {
			if (auto* samples = event->getValue()) {
				this->drawWaveform(*samples);
			}
		});
		m_sampleTaskListener.setFilter(info.first);

		m_drawNode = CCNode::create();
		m_drawNode->setAnchorPoint(ccp(0.5f, 0.5f));
		m_drawNode->setContentSize(m_widgetSize);

		const auto unplayedColor = geode::cocos::cc4bFromHexString("ffad31", false, true).unwrap();
		const auto playedColor = geode::cocos::cc4bFromHexString("ff", false, true).unwrap();

		auto* overlay = CCLayerColor::create(playedColor, targetWidth, barHeight + 1);
		overlay->setBlendFunc({GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA});
		overlay->ignoreAnchorPointForPosition(false);
		overlay->setAnchorPoint(ccp(0.0f, 0.5f));
		overlay->setZOrder(10);
		m_drawNode->addChildAtPosition(overlay, Anchor::Center, ccp(-targetWidth / 2.f, 0.f));
		m_overlayNode = overlay;

		overlay = CCLayerColor::create(unplayedColor, targetWidth, barHeight + 1);
		overlay->setBlendFunc({GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA});
		overlay->ignoreAnchorPointForPosition(false);
		overlay->setAnchorPoint(ccp(1.0f, 0.5f));
		overlay->setZOrder(10);
		m_drawNode->addChildAtPosition(overlay, Anchor::Center, ccp(targetWidth / 2.f, 0.f));
		m_overlay2Node = overlay;

		this->schedule(schedule_selector(DetailedAudioPreviewPopup::update), 1.f / 60.f);

		uibuilder::Build<CCNode>::create()
			.anchorPoint(0.5, 0.5)
			.layout(ColumnLayout::create()
				->setAxisAlignment(AxisAlignment::Start)
				->setAutoGrowAxis(m_widgetSize.height)
				->setGrowCrossAxis(true)
				->setAxisReverse(true)
				->setGap(0.f)
			)
			.child(uibuilder::Build<CCNode>::create()
				.zOrder(-2)
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Start)
					->setGap(0.f)
					->setAutoScale(false)
					->setGrowCrossAxis(true)
				)
				.child(uibuilder::Build<CCSprite>::create("cursor.png"_spr)
					.anchorPoint(0.5f, 0.f)
					.store(m_cursorNode)
				)
				.updateLayout()
			)
			.child(m_drawNode)
			.child(uibuilder::Build<CCNode>::create()
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Between)
					->setGap(0.f)
					->setGrowCrossAxis(true)
				)
				.child(uibuilder::Build<CCMenu>::create()
					.layout(RowLayout::create()
						->setAxisAlignment(AxisAlignment::Start)
						->setAutoGrowAxis(1.f)
						->setGap(0.f)
					)
					.child(uibuilder::Build<CCSprite>::createSpriteName("GJ_undoBtn_001.png")
						.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onGoToStartOffset))
					)
					.child(uibuilder::Build<CCSprite>::createSpriteName("GJ_playMusicBtn_001.png")
						.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onPlay))
					)
					.updateLayout()
				)
				.child(uibuilder::Build<CCLabelBMFont>::create("hello", "chatFont.fnt")
					.anchorPoint(1.f, 0.5f)
					.store(m_timeLabel)
				)
				.updateLayout()
			)
			.child(uibuilder::Build<CCNode>::create()
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Start)
					->setGrowCrossAxis(true)
					->setAutoScale(false)
				)
				.child(uibuilder::Build<CCLabelBMFont>::create("Start Offset", "bigFont.fnt")
					.anchorPoint(0.f, 0.5f)
					.scale(0.5f)
				)
				.child(uibuilder::Build<geode::TextInput>::create(50.f, "x:xx", "chatFont.fnt")
					.with([](geode::TextInput* node) {
						node->setString("0:00");
						node->setFilter("0123456789:.");
						node->setCallback([](auto const& data) {
							log::info("got it {}", data);
						});
					})
					.store(m_startOffsetInput)
				)
				.updateLayout()
			)
			.updateLayout()
			.with([this](auto* node) {
				m_mainLayer->addChildAtPosition(node, Anchor::Center);
			})
		;

		// cheat layouts
		m_cursorNode->setPositionX(0.f);

		this->visualSeekTo(0.f);

		uibuilder::Build<ButtonSprite>::create("Use")
			.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onUseSong))
			.anchorPoint(0.5f, 0.f)
			.with([this](auto* node) {
				m_buttonMenu->addChildAtPosition(node, Anchor::Bottom, ccp(0.f, 10.f));
			})
		;

        return true;
    }

	void drawWaveform(const std::span<float> samples) {
		auto* parent = m_drawNode->getParent();

		auto* drawNode = CCDrawNode::create();
		drawNode->setZOrder(-1);
		parent->addChild(drawNode);
		// transfer the overlay nodes
		for (auto* child : CCArrayExt<CCNode*>(m_drawNode->getChildren())) {
			child->retain();
			child->removeFromParent();
			drawNode->addChild(child);
			child->release();
		}

		m_drawNode->removeFromParent();
		m_drawNode = drawNode;
		
		drawNode->setAnchorPoint(ccp(0.5f, 0.5f));
		drawNode->setContentSize(m_widgetSize);
		drawNode->drawRect(ccp(0, 0), m_widgetSize, ccc4f(0.f, 0.f, 0.f, 1.f), 0.f, ccc4f(0.f, 0.f, 0.f, 0.f));
		for (float x = 0.f; x < m_widgetSize.width; x += m_barSize.width) {
			int index = static_cast<int>(x / m_widgetSize.width * samples.size());
			float height = samples[index] * m_barSize.height;
			auto tl = ccp(x, (m_barSize.height + height) / 2.f);
			auto br = ccp(x + m_barSize.width, (m_barSize.height - height) / 2.f);
			auto tr = ccp(br.x, tl.y);
			auto bl = ccp(tl.x, br.y);
			CCPoint points[] = {tl, tr, br, bl};
			drawNode->drawPolygon(points, 4, ccc4f(1.f, 1.f, 1.f, 0.f), 0.f, ccc4f(0.f, 0.f, 1.f, 1.f));
			// TODO: properly draw a 2 pixel rectangle because cocos is stupid
			// and forces some stupid outline
			points[0].x += m_barSize.width;
			points[1].x += m_barSize.width;
			points[2].x += m_barSize.width;
			points[3].x += m_barSize.width;
			drawNode->drawPolygon(points, 4, ccc4f(0.f, 0.f, 0.f, 1.f), 0.f, ccc4f(0.f, 0.f, 1.f, 1.f));
		}

		parent->updateLayout();
	}

	bool m_seekingOffset = false;
	void seekOffsetTo(float percent) {
		percent = std::clamp(percent, 0.f, 1.f);
		m_startOffset = percent * m_songLength;
		m_cursorNode->setPositionX(percent * m_widgetSize.width);
		{
			auto time = fmt::format("{}:{:05.2f}", int(m_startOffset) / 60, std::fmod(m_startOffset, 60.f));
			m_startOffsetInput->setString(time);
		}
	}

	bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
		if (auto [touchLocation, inside] = testTouchInside(touch, m_drawNode); inside) {
			m_seekToRequested = true;
			m_seekToPercent = touchLocation.x / m_widgetSize.width;
			m_seeking = true;
			this->visualSeekTo(m_seekToPercent);
			return true;
		} else if (auto [_, inside] = testTouchInside(touch, m_cursorNode); inside) {
			m_seekingOffset = true;
			auto [touchLocation, _2] = testTouchInside(touch, m_drawNode);
			this->seekOffsetTo(touchLocation.x / m_widgetSize.width);
		}
		return Popup::ccTouchBegan(touch, event);
	}

	void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
		if (m_seeking) {
			auto location = touch->getLocation();
			auto nodeLocation = m_drawNode->convertToNodeSpace(location);
			auto size = m_drawNode->getContentSize();
			m_seekToPercent = std::clamp(nodeLocation.x / size.width, 0.f, 1.f);
			this->visualSeekTo(m_seekToPercent);
		} else if (m_seekingOffset) {
			auto [touchLocation, _] = testTouchInside(touch, m_drawNode);
			this->seekOffsetTo(touchLocation.x / m_widgetSize.width);
		} else {
			Popup::ccTouchMoved(touch, event);
		}
	}

	void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
		m_seekingOffset = false;
		if (m_seeking) {
			m_seeking = false;
			m_seekToRequested = true;
		} else {
			Popup::ccTouchEnded(touch, event);
		}
	}

	bool m_seeking = false;
	float m_seekToPercent = 0.f;
	bool m_seekToRequested = false;

	void visualSeekTo(float percent) {
		m_overlayNode->setScaleX(percent);
		m_overlay2Node->setScaleX(1.f - percent);
		m_timeLabel->setString(fmt::format("{} / {}", formatSeconds(percent * m_songLength), formatSeconds(m_songLength)).c_str());
	}

	void update(float dt) override {
		auto* engine = FMODAudioEngine::sharedEngine();
		if (engine->isMusicPlaying(0)) {
			auto time = static_cast<float>(engine->getMusicTimeMS(0));
			if (!m_seeking) {
				if (m_seekToRequested) {
					engine->setMusicTimeMS(static_cast<unsigned int>(m_songLength * m_seekToPercent * 1000.f), false, 0);
					m_seekToRequested = false;
				} else {
					this->visualSeekTo((time / 1000.f) / m_songLength);
				}
			}
		}
	}

	void onPlay(CCObject*) {
		auto path = MusicDownloadManager::sharedState()->pathForSong(m_songId);
		auto* engine = FMODAudioEngine::sharedEngine();
		if (engine->isMusicPlaying(0)) {
			engine->pauseMusic(0);
		} else {
			engine->playMusic(path, false, 0.f, 2);
		}
	}

	void onGoToStartOffset(CCObject*) {
		auto* engine = FMODAudioEngine::sharedEngine();
		engine->setMusicTimeMS(static_cast<unsigned int>(m_startOffset * 1000.f), false, 0);
		this->visualSeekTo(m_startOffset / m_songLength);
	}

	void onUseSong(CCObject*) {
		auto* editor = LevelEditorLayer::get();
		if (editor && m_parentWidget) {
			editor->m_levelSettings->m_songOffset = m_startOffset;
			m_parentWidget->onSelect(nullptr);
		}
		this->onClose(nullptr);
	}

public:
    static DetailedAudioPreviewPopup* create(int text, CustomSongWidget* parent) {
        auto ret = new DetailedAudioPreviewPopup();
        if (ret->initAnchored(360.f, 240.f, text, parent)) {
            ret->autorelease();
        } else {
        	CC_SAFE_DELETE(ret);
		}
		return ret;
    }
};

#include <Geode/modify/CustomSongWidget.hpp>
class $modify(CustomSongWidget) {
	void onPlayback(CCObject*) {
		DetailedAudioPreviewPopup::create(m_songInfoObject->m_songID, this)->show();
	}
};
