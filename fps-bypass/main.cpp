#include <Geode/Geode.hpp>
#include <Geode/Modify.hpp>
#include <chrono>

USE_GEODE_NAMESPACE();

void update_fps() {
	if (Mod::get()->getSettingValue<bool>("enabled")) {
		auto* app = CCApplication::sharedApplication();
		if (app->getVerticalSyncEnabled()) {
			log::debug("toggled vsync");
			app->toggleVerticalSync(false);
			GameManager::sharedState()->setGameVariable("0030", false);
		}
		const int value = Mod::get()->getSettingValue<int>("fps-value");
		log::debug("updating fps to {}", value);
		app->setAnimationInterval(1.0 / static_cast<double>(value));
	}
}

class $modify(AppDelegate) {
	void applicationWillEnterForeground() {
		AppDelegate::applicationWillEnterForeground();
		update_fps();
	}
};

$execute {
	listenForSettingChanges<IntSetting>("fps-value", [](auto) {
		update_fps();
	});
	listenForSettingChanges<BoolSetting>("enabled", [](auto setting) {
		if (setting->getValue()) {
			update_fps();
		} else {
			CCApplication::sharedApplication()->setAnimationInterval(1.0 / 60.0);
		}
	});
}

// ui

std::chrono::time_point<std::chrono::high_resolution_clock> previous_frame, last_update;
float frame_time_sum = 0.f;
int frame_count = 0;

class $modify(MyPlayLayer, PlayLayer) {
	CCLabelBMFont* fps_label;

	bool init(GJGameLevel* level) {
		if (!PlayLayer::init(level)) return false;

		if (Mod::get()->getSettingValue<bool>("fps-label")) {
			auto* label = CCLabelBMFont::create("69 fps", "bigFont.fnt");
			m_fields->fps_label = label;
			this->addChild(label);
			
			const auto win_size = CCDirector::sharedDirector()->getWinSize();
			label->setPosition(win_size.width - 3.f, win_size.height - 3.f);
			label->setAnchorPoint(ccp(1, 0.8));
			label->setZOrder(999);
			label->setOpacity(128);
			label->setScale(0.4f);
			label->setTag(1234567);

			this->schedule(schedule_selector(MyPlayLayer::update_label));
		}

		return true;
	}

	void update_label(float) {
		const auto now = std::chrono::high_resolution_clock::now();

		const std::chrono::duration<float> diff = now - previous_frame;
		frame_time_sum += diff.count();
		frame_count++;

		if (std::chrono::duration<float>(now - last_update).count() > 1.0f) {
			last_update = now;
			const auto fps = static_cast<int>(std::roundf(static_cast<float>(frame_count) / frame_time_sum));
			frame_time_sum = 0.f;
			frame_count = 0;
			m_fields->fps_label->setString((std::to_string(fps) + " FPS").c_str());
		}

		previous_frame = now;
	}
};