#include <Geode/Geode.hpp>
#include <UIBuilder.hpp>
#include <span>
#include <cstdio>
#include <arc/future/UtilPollables.hpp>
#include "samples.hpp"
#include "waveform.hpp"

using namespace geode::prelude;
using uibuilder::Build;

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

class DetailedAudioPreviewPopup : public geode::Popup {
protected:
	int m_songID;
	CCLabelBMFont* m_timeLabel;
	float m_songLength = 0.f;
	float m_startOffset = 0.f;
	geode::TextInput* m_startOffsetInput;
	CCSprite* m_cursorNode;
	CCSize m_widgetSize;
	CCSize m_barSize;
	CustomSongWidget* m_parentWidget = nullptr;
	CCMenuItemSpriteExtra* m_playButton = nullptr;
	bool m_isAlreadyChosenSong = false;
	WaveformNode* m_waveformNode;

	async::TaskHolder<typename SampleInfo::SampleFuture::Output> m_sampleListener;

	bool init(SongInfoObject* songInfo, CustomSongWidget* parent) {
		if (!Popup::init(CCDirector::get()->getWinSize().width / 2.f + 75.5f, 240.f))
			return false;

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

		auto [sampleFuture, songLength] = SampleInfo::fromFile(path, targetWidth / pixel);
		m_songLength = songLength;

		m_sampleListener.spawn(std::move(sampleFuture), [this](SampleInfo::SampleFuture::Output res) {
			if (GEODE_UNWRAP_IF_OK(samples, res)) {
				this->drawWaveform(samples);
			} else {
				FLAlertLayer::create("Error", "Failed to load song information", "OK")->show();
			}
		});

		m_waveformNode = WaveformNode::create(m_widgetSize);

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
			.child(m_waveformNode)
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
						node->setCallback([this](std::string const& str) {
							this->onTimeInputChange(str);
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
		m_waveformNode->drawSamples(samples);
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
		if (auto [touchLocation, inside] = testTouchInside(touch, m_waveformNode); inside) {
			m_seekToRequested = true;
			m_seekToPercent = touchLocation.x / m_widgetSize.width;
			m_seeking = true;
			this->visualSeekTo(m_seekToPercent);
			return true;
		} else if (auto [_, inside] = testTouchInside(touch, m_cursorNode); inside) {
			m_seekingOffset = true;
			auto [touchLocation, _2] = testTouchInside(touch, m_waveformNode);
			this->seekOffsetTo(touchLocation.x / m_widgetSize.width);
		}
		return Popup::ccTouchBegan(touch, event);
	}

	void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
		if (m_seeking) {
			auto location = touch->getLocation();
			auto nodeLocation = m_waveformNode->convertToNodeSpace(location);
			auto size = m_waveformNode->getContentSize();
			m_seekToPercent = std::clamp(nodeLocation.x / size.width, 0.f, 1.f);
			this->visualSeekTo(m_seekToPercent);
		} else if (m_seekingOffset) {
			auto [touchLocation, _] = testTouchInside(touch, m_waveformNode);
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
		m_waveformNode->setPercent(percent);
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

	void onTimeInputChange(std::string const& str) {
		// try to parse it, ignore if it fails
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
		if (ret->init(info, parent)) {
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
