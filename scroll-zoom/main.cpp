#include <Geode/Geode.hpp>
#include <Geode/Modify.hpp>
#include <numbers>

using namespace geode::prelude;

class $modify(EditorUI) {
	void scrollWheel(float y, float x) {
		auto kb = CCDirector::sharedDirector()->getKeyboardDispatcher();
		if (kb->getControlKeyPressed()) {
			auto zoom = m_editorLayer->m_objectLayer->getScale();
			zoom = std::pow(std::numbers::e, std::log(std::max(zoom, 0.001f)) - y * 0.015f);
			this->updateZoom(zoom);
		} else {
			auto layer = m_editorLayer->m_objectLayer;
			const float mult = 2.f;
			if (kb->getShiftKeyPressed()) {
				layer->setPositionX(layer->getPositionX() - y * mult);
			} else {
				layer->setPositionY(layer->getPositionY() + y * mult);
			}
			EditorUI::scrollWheel(0.f, 0.f);
		}
	}
};
