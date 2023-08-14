#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameSoundManager.hpp>

using namespace geode::prelude;

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
			// recreate what togglePracticeMode does, but dont play practice mode song
			m_isPracticeMode = toggle;
			if (!GameManager::get()->getGameVariable("0071")) {
				m_UILayer->toggleCheckpointsMenu(toggle);
			}
			this->startMusic();
			this->stopActionByTag(18);
		} else {
			PlayLayer::togglePracticeMode(toggle);
		}
	}

	static void onModify(auto& self) {
		(void) self.setHookPriority("PlayLayer::togglePracticeMode", 9999);
	}
};
