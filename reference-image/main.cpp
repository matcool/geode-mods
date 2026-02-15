#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace std::string_literals;

#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUI, EditorUI) {
	struct Fields {
		GameObject* m_object = nullptr;
		CCSprite* m_sprite = nullptr;
	};

	void onButton(CCObject*) {
		if (this->getSelectedObjects()->count() == 1) {
			auto* object = CCArrayExt<GameObject*>(this->getSelectedObjects())[0];
			m_fields->m_object = object;
			geode::async::spawn(
				utils::file::pick(file::PickMode::OpenFile, {}),
				[this, object](Result<std::optional<std::filesystem::path>> result) {
					auto opt = std::move(result).unwrapOrDefault();
					if (!opt) return;
					auto path = std::move(opt).value();

					if (m_fields->m_sprite) {
						m_fields->m_sprite->removeFromParent();
						m_fields->m_sprite = nullptr;
					}
					#ifdef GEODE_IS_WINDOWS
					try {
						(void) path.string();
					} catch (...) {
						// copy file to mod temp dir, which is guaranteed to be within the codepage
						auto newPath = Mod::get()->getTempDir() / ("temp_ref_img"s + path.extension().string());
						std::filesystem::copy_file(path, newPath, std::filesystem::copy_options::overwrite_existing);
						path = newPath;
					}
					#endif
					auto* sprite = CCSprite::create(geode::utils::string::pathToString(path).c_str());
					if (sprite && !sprite->getUserObject("geode.texture-loader/fallback")) {
						m_fields->m_sprite = sprite;
						m_editorLayer->m_objectLayer->addChild(sprite);
					} else {
						object->setVisible(true);
						FLAlertLayer::create("Error", "Failed to load image", "OK")->show();
					}
				}
			);
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
			sprite->setColor(object->m_isSelected ? ccc3(0, 255, 0) : ccc3(255, 255, 255));
			object->setVisible(false);

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
