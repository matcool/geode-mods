#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>
class $modify(EditorUI) {
	void scrollWheel(float y, float x) {
		auto kb = CCDirector::sharedDirector()->getKeyboardDispatcher();
		if (kb->getControlKeyPressed()) {
			auto zoom = m_editorLayer->m_objectLayer->getScale();
			const float mult = 0.015f * Mod::get()->getSettingValue<double>("zoom-mult");
			zoom = std::max(0.05, zoom * std::pow(2.7, y * -mult));
			this->updateZoom(zoom);
		} else {
			auto layer = m_editorLayer->m_objectLayer;
			const float mult = Mod::get()->getSettingValue<double>("scroll-mult");
			if (kb->getShiftKeyPressed()) {
				layer->setPositionX(layer->getPositionX() - y * mult);
			} else {
				layer->setPositionY(layer->getPositionY() + y * mult);
			}
			EditorUI::scrollWheel(0.f, 0.f);
		}
	}
};
