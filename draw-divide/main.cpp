#include <Geode/Geode.hpp>

using namespace geode::prelude;

double delta_count = 0.0;
bool enabled = true;

#include <Geode/modify/CCDirector.hpp>
class $modify(CCDirector) {
	void drawScene() {
		// disable for first 300 frames of game being open
		if (!enabled || this->getTotalFrames() < 300) {
			return CCDirector::drawScene();
		}

		// always target the fps set in settings
		const double target_delta = 1.0 / Mod::get()->getSettingValue<int64_t>("fps");

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