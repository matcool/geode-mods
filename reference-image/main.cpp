#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUI, EditorUI) {
	GameObject* m_object = nullptr;
	CCSprite* m_sprite = nullptr;

	void onButton(CCObject*) {
		if (this->getSelectedObjects()->count() == 1) {
			auto* object = CCArrayExt<GameObject*>(this->getSelectedObjects())[0];
			m_fields->m_object = object;
			utils::file::pickFile(file::PickMode::OpenFile, {}, [this, object](ghc::filesystem::path const& path) {
				if (m_fields->m_sprite) {
					m_fields->m_sprite->removeFromParent();
					m_fields->m_sprite = nullptr;
				}
				auto* sprite = CCSprite::create(path.string().c_str());
				if (sprite) {
					m_fields->m_sprite = sprite;
					m_editorLayer->m_objectLayer->addChild(sprite);
				} else {
					FLAlertLayer::create("Error", "Invalid image", "OK")->show();
				}
			});
		} else {
			FLAlertLayer::create("Info", "You must select exactly 1 object", "OK")->show();
		}
	}

	void myUpdate(float) {
		if (m_fields->m_sprite && m_fields->m_object) {
			// check if the object still exists
			if (!m_editorLayer->m_objects->containsObject(m_fields->m_object)) {
				m_fields->m_object = nullptr;
				m_fields->m_sprite->removeFromParent();
				m_fields->m_sprite = nullptr;
				return;
			}
			auto* sprite = m_fields->m_sprite;
			auto* object = m_fields->m_object;
			sprite->setPosition(object->getPosition());
			sprite->setScaleX(object->getScaleX());
			sprite->setScaleY(object->getScaleY());
			sprite->setRotation(object->getRotation());

			sprite->setOpacity(object->getOpacity());
			auto* batch = object->getParent();
			if (batch) {
				sprite->setZOrder(batch->getZOrder() - 1);
			}
		}
	}

	void createMoveMenu() {
		EditorUI::createMoveMenu();
		auto* btn = this->getSpriteButton("button.png"_spr, menu_selector(MyEditorUI::onButton), nullptr, 0.9f);
		m_editButtonBar->m_buttonArray->addObject(btn);
		// TODO: replace with reloadItemsInNormalSize
		auto rows = GameManager::sharedState()->getIntGameVariable("0049");
		auto cols = GameManager::sharedState()->getIntGameVariable("0050");
		m_editButtonBar->reloadItems(rows, cols);

		this->schedule(schedule_selector(MyEditorUI::myUpdate), 0.05f);
	}
};
