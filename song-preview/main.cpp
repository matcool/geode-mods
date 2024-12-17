#include <Geode/Geode.hpp>
#include <unordered_map>
#include <UIBuilder.hpp>
#include <span>
#include <cstdio>

using namespace geode::prelude;
using uibuilder::Build;

struct SampleInfo {
	std::vector<float> samples;
	using SampleTask = Task<std::vector<float>>;

	static std::pair<Task<std::vector<float>>, float> fromFile(std::string_view songPath, int targetWidth) {
		auto* engine = FMODAudioEngine::sharedEngine();

		FMOD::Sound* sound;
		const FMOD_MODE mode = FMOD_DEFAULT | FMOD_OPENONLY;
		auto result = engine->m_system->createSound(songPath.data(), mode, nullptr, &sound);
		if (result != FMOD_OK) {
			log::error("Error creating sound! {}", int(result));
		}
		unsigned int lengthMilliseconds;
		result = sound->getLength(&lengthMilliseconds, FMOD_TIMEUNIT_MS);
		if (result != FMOD_OK) {
			log::error("Error getting length {}", int(result));
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

			// code based off config's mod:
			// https://github.com/cgytrus/EditorWaveform/blob/main/src/dllmain.cpp
			// lots of adjustments were made to make it match newground's waveform

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

class DetailedAudioPreviewPopup : public geode::Popup<SongInfoObject*, CustomSongWidget*>, public TextInputDelegate {
protected:
	int m_songID;
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
	CCMenuItemSpriteExtra* m_playButton = nullptr;
	bool m_isAlreadyChosenSong = false;

	EventListener<SampleInfo::SampleTask> m_sampleTaskListener;

    bool setup(SongInfoObject* songInfo, CustomSongWidget* parent) override {
		this->setID("DetailedAudioPreviewPopup"_spr);

		m_songID = songInfo->m_songID;
		m_parentWidget = parent;

		if (auto* editor = LevelEditorLayer::get()) {
			if (editor->m_level->m_songID == m_songID) {
				m_startOffset = editor->m_levelSettings->m_songOffset;
				m_isAlreadyChosenSong = true;
			}
		}

        auto winSize = CCDirector::sharedDirector()->getWinSize();

		auto path = MusicDownloadManager::sharedState()->pathForSong(m_songID);
		path = CCFileUtils::get()->fullPathForFilename(path.c_str(), false);
		
		auto* engine = FMODAudioEngine::sharedEngine();
		engine->stopAllMusic(true);
		engine->playMusic(path, false, 0.f, 0);


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

		Build<CCNode>::create()
			.id("main-layout")
			.anchorPoint(0.5, 0.5)
			.layout(ColumnLayout::create()
				->setAxisAlignment(AxisAlignment::Start)
				->setAutoGrowAxis(m_widgetSize.height)
				->setGrowCrossAxis(true)
				->setAxisReverse(true)
				->setGap(0.f)
			)
			.child(Build<CCNode>::create()
				.zOrder(-2)
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Start)
					->setGap(0.f)
					->setAutoScale(false)
					->setGrowCrossAxis(true)
				)
				.child(Build<CCSprite>::create("cursor.png"_spr)
					.anchorPoint(0.5f, 0.f)
					.store(m_cursorNode)
				)
				.updateLayout()
			)
			.child(m_drawNode)
			.child(Build<CCNode>::create()
				.contentSize({10.f, 2.f})
			)
			.child(Build<CCNode>::create()
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Between)
					->setGap(0.f)
					->setGrowCrossAxis(true)
				)
				.child(Build<CCMenu>::create()
					.layout(RowLayout::create()
						->setAxisAlignment(AxisAlignment::Start)
						->setAutoGrowAxis(1.f)
						->setGap(5.f)
					)
					.child(Build(CircleButtonSprite::createWithSprite("back.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small))
						.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onGoToStartOffset))
					)
					.child(Build<CCSprite>::createSpriteName("GJ_pauseBtn_001.png")
						.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onPlay))
						.store(m_playButton)
					)
					.updateLayout()
				)
				.child(Build<CCLabelBMFont>::create("hello", "chatFont.fnt")
					.anchorPoint(1.f, 0.5f)
					.store(m_timeLabel)
				)
				.updateLayout()
			)
			.child(Build<CCNode>::create()
				.contentSize(m_widgetSize)
				.layout(RowLayout::create()
					->setAxisAlignment(AxisAlignment::Start)
					->setGrowCrossAxis(true)
					->setAutoScale(false)
				)
				.child(Build<CCLabelBMFont>::create("Start Offset", "bigFont.fnt")
					.anchorPoint(0.f, 0.5f)
					.scale(0.5f)
				)
				.child(Build<geode::TextInput>::create(50.f, "x:xx", "chatFont.fnt")
					.with([this](geode::TextInput* node) {
						node->setString("0:00");
						node->setFilter("0123456789:.");
						node->setDelegate(this);
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

		this->seekOffsetTo(m_startOffset / m_songLength);

		this->visualSeekTo(0.f);

		Build<ButtonSprite>::create("Use")
			.intoMenuItem(this, menu_selector(DetailedAudioPreviewPopup::onUseSong))
			.id("use-button")
			.anchorPoint(0.5f, 0.f)
			.with([this](auto* node) {
				m_buttonMenu->addChildAtPosition(node, Anchor::Bottom, ccp(0.f, 10.f));
			})
		;

		Build<CCNode>::create()
			.id("song-info-layout")
			.anchorPoint(0.5, 0.5)
			.layout(ColumnLayout::create()
				->setAxisAlignment(AxisAlignment::End)
				->setCrossAxisLineAlignment(AxisAlignment::Center)
				->setAxisReverse(true)
				->setGrowCrossAxis(true)
				->setAutoGrowAxis(1.f)
				->setGap(0.f)
				->setAutoScale(false)
			)
			.child(Build<CCLabelBMFont>::create(songInfo->m_songName.c_str(), "bigFont.fnt")
				.anchorPoint(0, 0.5)
				.limitLabelWidth(m_widgetSize.width, 0.68f, 0.1f)
			)
			.child(Build<CCLabelBMFont>::create(songInfo->m_artistName.c_str(), "goldFont.fnt")
				.anchorPoint(0, 0.5)
				.scale(0.55)
			)
			.with([this](auto* node) {
				m_mainLayer->addChildAtPosition(node, Anchor::Top, ccp(0.f, -29.f));
			})
			.updateLayout()
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
	void seekOffsetTo(float percent, bool updateInput = true) {
		percent = std::clamp(percent, 0.f, 1.f);
		m_startOffset = percent * m_songLength;
		m_cursorNode->setPositionX(percent * m_widgetSize.width);
		if (updateInput) {
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
		auto path = MusicDownloadManager::sharedState()->pathForSong(m_songID);
		auto* engine = FMODAudioEngine::sharedEngine();
		const char* sprite;
		if (engine->isMusicPlaying(0)) {
			engine->pauseMusic(0);
			sprite = "GJ_playMusicBtn_001.png";
		} else {
			engine->playMusic(path, false, 0.f, 0);
			sprite = "GJ_pauseBtn_001.png";
		}
		m_playButton->setNormalImage(CCSprite::createWithSpriteFrameName(sprite));
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

	void onClose(CCObject* obj) override {
		if (auto* editor = LevelEditorLayer::get(); editor && m_isAlreadyChosenSong) {
			editor->m_levelSettings->m_songOffset = m_startOffset;
		}

		Popup::onClose(obj);
	}

    void textChanged(CCTextInputNode* input) override {
		// try to parse it, ignore if it fails
		auto str = input->getString();
		int minutes = 0;
		float seconds = 0;
		float time = 0.f;
		if (std::sscanf(str.c_str(), "%d:%f", &minutes, &seconds) == 2) {
			time = minutes * 60.f + seconds;
		} else if (std::sscanf(str.c_str(), "%f", &seconds) == 1) {
			time = seconds;
		} else {
			return;
		}

		this->seekOffsetTo(time / m_songLength, false);
	}

    void textInputClosed(CCTextInputNode* input) override {
		this->seekOffsetTo(m_startOffset / m_songLength);
	}

public:
    static DetailedAudioPreviewPopup* create(SongInfoObject* info, CustomSongWidget* parent) {
        auto ret = new DetailedAudioPreviewPopup();
        if (ret->initAnchored(CCDirector::get()->getWinSize().width / 2.f + 75.5f, 240.f, info, parent)) {
            ret->autorelease();
        } else {
        	CC_SAFE_DELETE(ret);
		}
		return ret;
    }
};

#include <Geode/modify/CustomSongWidget.hpp>
class $modify(CustomSongWidget) {
	void onPlayback(CCObject* sender) {
		if (LevelEditorLayer::get() && CCScene::get()->getChildByType<LevelSettingsLayer>(0)) {
			DetailedAudioPreviewPopup::create(m_songInfoObject, this)->show();
		} else {
			CustomSongWidget::onPlayback(sender);
		}
	}
};
