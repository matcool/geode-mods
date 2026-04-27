#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <nodes.hpp>

using namespace geode;
using namespace cocos2d;
using geode::cocos::CCArrayExt;

class CircleToolPopup : public geode::Popup {
public:
	static float m_angle;
	static float m_step;
	static float m_fat;
	CCLabelBMFont* m_label = nullptr;

	static CircleToolPopup* create() {
		auto* node = new CircleToolPopup();
		if (node->init()) {
			node->autorelease();
			return node;
		} else {
			delete node;
			return nullptr;
		}
	}

	bool init() override {
		if (!Popup::init(300, 220)) return false;

		this->setTitle("Circle Tool");

		auto* layer = m_mainLayer;
		auto* menu = m_buttonMenu;

		layer->addChildAtPosition(
			NodeFactory<CCLabelBMFont>::start("Arc", "goldFont.fnt")
				.setScale(0.75f),
			Anchor::Center, ccp(-60, 64)
		);
		auto angle_input = geode::TextInput::create(60.f, "");
		angle_input->setCommonFilter(CommonFilter::Float);
		angle_input->setString(fmt::to_string(m_angle));
		angle_input->setCallback([this](std::string const& str) {
			m_angle = geode::utils::numFromString<float>(str).unwrapOr(m_angle);
			this->update_labels();
		});
		layer->addChildAtPosition(angle_input, Anchor::Center, ccp(-60, 38));

		layer->addChildAtPosition(
			NodeFactory<CCLabelBMFont>::start("Step", "goldFont.fnt")
				.setScale(0.75f),
			Anchor::Center, ccp(60, 64)
		);
		auto step_input = geode::TextInput::create(60.f, "");
		step_input->setCommonFilter(CommonFilter::Float);
		step_input->setString(fmt::to_string(m_step));
		step_input->setCallback([this](std::string const& str) {
			m_step = geode::utils::numFromString<float>(str).unwrapOr(m_step);
			if (m_step == 0.f) m_step = 1.f;
			this->update_labels();
		});
		layer->addChildAtPosition(step_input, Anchor::Center, ccp(60, 38));

		layer->addChildAtPosition(
			NodeFactory<CCLabelBMFont>::start("Squish", "goldFont.fnt")
				.setScale(0.75f),
			Anchor::Center, ccp(60, 10)
		);
		auto fat_input = geode::TextInput::create(60.f, "");
		fat_input->setCommonFilter(CommonFilter::Float);
		fat_input->setString(fmt::to_string(m_fat));
		fat_input->setCallback([this](std::string const& str) {
			m_fat = geode::utils::numFromString<float>(str).unwrapOr(m_fat);
			this->update_labels();
		});
		layer->addChildAtPosition(fat_input, Anchor::Center, ccp(60, -16));

		float button_width = 68;
		menu->addChildAtPosition(
			CCMenuItemSpriteExtra::create(
				ButtonSprite::create("Apply", button_width, true, "goldFont.fnt", "GJ_button_01.png", 0, 0.75f),
				this, menu_selector(CircleToolPopup::on_apply)
			),
			Anchor::Center, ccp(button_width / 2.f + 20, -85)
		);

		menu->addChildAtPosition(
			CCMenuItemSpriteExtra::create(
				ButtonSprite::create("Cancel", button_width, true, "goldFont.fnt", "GJ_button_01.png", 0, 0.75f),
				this, menu_selector(CircleToolPopup::onClose)
			),
			Anchor::Center, ccp(button_width / -2.f - 20, -85)
		);

		m_label = CCLabelBMFont::create("copies: 69\nobjects: 69420", "chatFont.fnt");
		m_label->setAlignment(kCCTextAlignmentLeft);
		layer->addChildAtPosition(m_label, Anchor::Center, ccp(-83, -41));
		this->update_labels();

		auto info_btn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this, menu_selector(CircleToolPopup::on_info)
		);
		menu->addChildAtPosition(info_btn, Anchor::TopRight, ccp(-19, -19));

		info_btn = CCMenuItemSpriteExtra::create(
			NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"))
				.setScale(0.6f),
			this, menu_selector(CircleToolPopup::on_info2)
		);
		menu->addChildAtPosition(info_btn, Anchor::Center, ccp(60, 10) + ccp(42, -1.5));

		return true;
	}

	void update_labels() {
		auto objs = GameManager::sharedState()->getEditorLayer()->m_editorUI->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		const auto obj_count = amt * objs->count();
		m_label->setString(fmt::format("Copies: {}\nObjects: {}", amt, obj_count).c_str());
	}

	void on_apply(CCObject*) {
		auto* editor = GameManager::sharedState()->getEditorLayer()->m_editorUI;
		auto objs = editor->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		if (objs && objs->count()) {
			const auto obj_count = objs->count() * amt;
			if (obj_count > 5000) {
				createQuickPopup("Warning",
					fmt::format("This will create <cy>{}</c> objects, are you sure?", obj_count),
					"Cancel", "Ok",
					[this](auto*, bool btn2) {
						if (btn2) {
							this->perform();
						}
					}
				);
			} else {
				perform();
			}
		}
	}

	void perform() {
		auto* editor = GameManager::sharedState()->getEditorLayer();
		auto* editor_ui = editor->m_editorUI;
		auto* objs = CCArray::create();
		const auto calc = [](float angle) {
			return -sinf(angle / 180.f * 3.141592f - 3.141592f / 2.f) * m_fat;
		};
		float off_acc = calc(0);
		for (float i = 1; i * m_step < m_angle; ++i) {
			editor_ui->onDuplicate(nullptr);
			auto selected = editor_ui->getSelectedObjects();
			editor_ui->rotateObjects(selected, m_step, {0.f, 0.f});

			const float angle = i * m_step;

			if (m_fat != 0.f) {
				float off_y = calc(angle) - off_acc;

				for (auto obj : CCArrayExt<GameObject*>(selected)) {
					editor_ui->moveObject(obj, {0, off_y});
				}

				off_acc = calc(angle);
			}

			// remove undo object for pasting the objs
			editor->m_undoObjects->removeLastObject();
			objs->addObjectsFromArray(selected);
		}
		editor->m_undoObjects->addObject(UndoObject::createWithArray(objs, UndoCommand::Paste));
		// second argument is ignoreSelectFilter
		editor_ui->selectObjects(objs, true);
		this->keyBackClicked();
	}

	void on_info(CCObject*) {
		FLAlertLayer::create(nullptr, "info",
			"This will repeatedly copy-paste and rotate the selected objects,\n"
			"rotating by <cy>Step</c> degrees each time, until it gets to <cy>Arc</c> degrees.",
			"ok", nullptr, 300.f
		)->show();
	}

	void on_info2(CCObject*) {
		FLAlertLayer::create(nullptr, "info",
			"This will move the selection up and down with the rotation angle, as to create an <cy>ellipse</c>.\n"
			"This number will control how much it goes up and down, thus controlling how <cg>\"squished\"</c> the circle is.\n"
			"This works best with a single object as the center, marked as <co>group parent</c>.",
			"ok", nullptr, 400.f
		)->show();
	}
};

float CircleToolPopup::m_angle = 180.0f;
float CircleToolPopup::m_step = 5.f;
float CircleToolPopup::m_fat = 0.f;


class $modify(MyEditorUI, EditorUI) {
	void on_circle_tool(CCObject*) {
		if (this->getSelectedObjects()->count()) {
			CircleToolPopup::create()->show();
		} else {
			FLAlertLayer::create("Info", "You must select some objects to use circle tool", "OK")->show();
		}
	}

	void createMoveMenu() {
		EditorUI::createMoveMenu();
		auto* btn = this->getSpriteButton("button.png"_spr, menu_selector(MyEditorUI::on_circle_tool), nullptr, 0.9f);
		m_editButtonBar->m_buttonArray->addObject(btn);
		auto rows = GameManager::sharedState()->getIntGameVariable("0049");
		auto cols = GameManager::sharedState()->getIntGameVariable("0050");
		// TODO: replace with reloadItemsInNormalSize
		m_editButtonBar->reloadItems(rows, cols);
	}
};
