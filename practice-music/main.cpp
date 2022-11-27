#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameSoundManager.hpp>

USE_GEODE_NAMESPACE();

bool lock_music = false;

class $modify(PlayLayer) {
	void startMusic() {
		bool is_practice = m_isPracticeMode;
		m_isPracticeMode = false;
		PlayLayer::startMusic();
		m_isPracticeMode = is_practice;
	}

	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		if (m_isPracticeMode && obj != m_antiCheatObject) {
			GameSoundManager::sharedManager()->stopBackgroundMusic();
		}
		PlayLayer::destroyPlayer(player, obj);
	}

	void togglePracticeMode(bool toggle) {
		if (!m_isPracticeMode && toggle) {
			lock_music = true;
		}
		PlayLayer::togglePracticeMode(toggle);
		if (lock_music) {
			lock_music = false;
			this->startMusic();
			this->stopActionByTag(18);
		}
	}
};

class $modify(GameSoundManager) {
	void playBackgroundMusic(std::string name, bool a, bool b) {
		if (!lock_music) {
			GameSoundManager::playBackgroundMusic(name, a, b);
		}
	}
};