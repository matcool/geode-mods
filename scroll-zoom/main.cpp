#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>
class $modify(EditorUI) {
	void scrollWheel(float y, float x) override {
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

#ifdef GEODE_IS_MOBILE
	struct Fields {
		Ref<CCTouch> m_anchorTouch;
		Ref<CCTouch> m_pinchTouch;
		float m_initialDistance = 0.f;
		float m_initialScale = 1.f;
		CCPoint m_initialCenter;
	};

	bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
		bool eat = EditorUI::ccTouchBegan(touch, event);
		if (eat || (m_editorLayer->m_playbackMode != PlaybackMode::Playing && m_fields->m_anchorTouch)) {
			if (m_fields->m_anchorTouch == nullptr) {
				m_fields->m_anchorTouch = touch;
			} else if (m_fields->m_pinchTouch == nullptr) {
				m_fields->m_pinchTouch = touch;
				m_fields->m_initialScale = m_editorLayer->m_objectLayer->getScale();
				auto pinchLoc = m_fields->m_pinchTouch->getLocation();
				auto anchorLoc = m_fields->m_anchorTouch->getLocation();
				m_fields->m_initialCenter = (pinchLoc + anchorLoc) / 2.f;
				auto distNow = pinchLoc.getDistance(anchorLoc);
				m_fields->m_initialDistance = distNow;
			}
			return true;
		}
		return eat;
	}
	void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
		if (m_editorLayer->m_playbackMode != PlaybackMode::Playing && m_fields->m_pinchTouch) {
			auto pinchLoc = m_fields->m_pinchTouch->getLocation();
			auto anchorLoc = m_fields->m_anchorTouch->getLocation();
			auto center = (pinchLoc + anchorLoc) / 2.f;
			auto distNow = pinchLoc.getDistance(anchorLoc);
			auto mult = m_fields->m_initialDistance / distNow;
			auto zoom = std::max(m_fields->m_initialScale / mult, 0.05f);
			if (std::isnan(zoom) || std::isinf(zoom)) {
				this->updateZoom(1.f);
				zoom = 1.f;
			} else {
				this->updateZoom(zoom);
			}
			auto diff = m_fields->m_initialCenter - center;
			auto* layer = m_editorLayer->m_objectLayer;
			layer->setPosition(layer->getPosition() - diff);

			m_fields->m_initialCenter = center;
		} else {
			return EditorUI::ccTouchMoved(touch, event);
		}
	}

	void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
		if (touch == m_fields->m_anchorTouch || touch == m_fields->m_pinchTouch) {
			m_fields->m_pinchTouch = nullptr;
			m_fields->m_anchorTouch = nullptr;
		}
		return EditorUI::ccTouchEnded(touch, event);
	}

#endif
};
