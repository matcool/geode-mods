#include <Geode/Geode.hpp>

using namespace geode::prelude;

double get_refresh_rate() {
	static const double refresh_rate = [] {
		DEVMODEA device_mode;
		memset(&device_mode, 0, sizeof(device_mode));
		device_mode.dmSize = sizeof(device_mode);
		device_mode.dmDriverExtra = 0;

		if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &device_mode) == 0) {
			return 60.0;
		} else {
			// TODO: see if there is a way to get the exact frequency?
			// like 59.940hz
			auto freq = device_mode.dmDisplayFrequency;
			// interlaced screens actually display twice of the reported frequency
			if (device_mode.dmDisplayFlags & DM_INTERLACED) {
				freq *= 2;
			}
			return static_cast<double>(freq);
		}
	}();
	return refresh_rate;
}

double delta_count = 0.0;
bool enabled = true;

#include <Geode/modify/CCDirector.hpp>
class $modify(CCDirector) {
	void drawScene() {
		// disable for first 300 frames of game being open
		if (!enabled || this->getTotalFrames() < 300) {
			return CCDirector::drawScene();
		}

		// always target the refresh rate of the monitor (* multiplier, defaults to ×1)
		const double target_fps = get_refresh_rate() * Mod::get()->getSettingValue<int64_t>("multiplier");;

		const double target_delta = 1.0 / target_fps;

		delta_count += this->getActualDeltaTime();

		// run full scene draw (glClear, visit) each time the counter is full
		if (delta_count >= target_delta) {
			// keep left over
			delta_count -= target_delta;
			return CCDirector::drawScene();
		}

		// otherwise, we only run updates

		// this line seems to create a speedhack
		// this->calculateDeltaTime_();

		if (!this->isPaused()) {
			this->CCDirector::getScheduler()->CCScheduler::update(this->CCDirector::getDeltaTime());
		}

		if (this->getNextScene()) {
			this->setNextScene();
		}
	}
};