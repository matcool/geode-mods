#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <nodes.hpp>

using namespace geode;
using namespace cocos2d;
using geode::cocos::CCArrayExt;

class CircleToolPopup : public FLAlertLayer, TextInputDelegate, FLAlertLayerProtocol {
public:
	static float m_angle;
	static float m_step;
	static float m_fat;
	CCLabelBMFont* m_label = nullptr;

	static auto* create() {
		auto* node = new (std::nothrow) CircleToolPopup();
		if (node && node->init()) {
			node->autorelease();
		} else {
			delete node;
			node = nullptr;
		}
		return node;
	}

	bool init() override {
		if (!this->initWithColor({0, 0, 0, 75})) return false;

		this->m_noElasticity = true;

		auto* director = CCDirector::sharedDirector();
		geode::cocos::handleTouchPriority(this);
		// director->getTouchDispatcher()->incrementForcePrio(69);
		this->registerWithTouchDispatcher();

		auto layer = CCLayer::create();
		auto menu = CCMenu::create();
		this->m_mainLayer = layer;
		this->m_buttonMenu = menu;
		
		layer->addChild(menu);
		this->addChild(layer);

		const float width = 300, height = 220;

		const CCPoint offset = director->getWinSize() / 2.f;
		auto bg = extension::CCScale9Sprite::create("GJ_square01.png");
		bg->setContentSize({width, height});
		bg->setPosition(offset);
		menu->setPosition(offset);
		bg->setZOrder(-2);
		layer->addChild(bg);

		auto close_btn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
			this, menu_selector(CircleToolPopup::on_close)
		);

		close_btn->setPosition(ccp(25.f, director->getWinSize().height - 25.f) - offset);
		menu->addChild(close_btn);

		this->setKeypadEnabled(true);


		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Circle Tool", "goldFont.fnt")
			.setPosition(ccp(0, 95) + offset)
			.setScale(0.75f)
		);

		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Arc", "goldFont.fnt")
			.setPosition(ccp(-60, 64) + offset)
			.setScale(0.75f)
		);
		auto angle_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		angle_input->set_value(m_angle);
		angle_input->setPosition(ccp(-60, 38) + offset);
		angle_input->callback = [&](FloatInputNode& input) {
			m_angle = input.get_value().value_or(m_angle);
			this->update_labels();
		};
		this->addChild(angle_input);
		
		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Step", "goldFont.fnt")
			.setPosition(ccp(60, 64) + offset)
			.setScale(0.75f)
		);
		auto step_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		step_input->set_value(m_step);
		step_input->setPosition(ccp(60, 38) + offset);
		step_input->callback = [&](FloatInputNode& input) {
			m_step = input.get_value().value_or(m_step);
			if (m_step == 0.f) m_step = 1.f;
			this->update_labels();
		};
		this->addChild(step_input);

		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Squish", "goldFont.fnt")
			.setPosition(ccp(60, 10) + offset)
			.setScale(0.75f)
		);
		auto fat_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		fat_input->set_value(m_fat);
		fat_input->setPosition(ccp(60, -16) + offset);
		fat_input->callback = [&](FloatInputNode& input) {
			m_fat = input.get_value().value_or(m_fat);
			this->update_labels();
		};
		this->addChild(fat_input);

		float button_width = 68;
		menu->addChild(
			make_factory(CCMenuItemSpriteExtra::create(
				ButtonSprite::create("Apply", button_width, true, "goldFont.fnt", "GJ_button_01.png", 0, 0.75f),
				this, menu_selector(CircleToolPopup::on_apply)
			))
			.setPosition(button_width / 2.f + 20, -85)
		);

		menu->addChild(
			make_factory(CCMenuItemSpriteExtra::create(
				ButtonSprite::create("Cancel", button_width, true, "goldFont.fnt", "GJ_button_01.png", 0, 0.75f),
				this, menu_selector(CircleToolPopup::on_close)
			))
			.setPosition(button_width / -2.f - 20, -85)
		);

		m_label = CCLabelBMFont::create("copies: 69\nobjects: 69420", "chatFont.fnt");
		m_label->setAlignment(kCCTextAlignmentLeft);
		m_label->setPosition(ccp(-83, -41) + offset);
		layer->addChild(m_label);
		this->update_labels();

		auto info_btn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this, menu_selector(CircleToolPopup::on_info)
		);
		info_btn->setPosition(ccp(-width / 2, height / 2) + ccp(20, -20));
		menu->addChild(info_btn);

		info_btn = CCMenuItemSpriteExtra::create(
			NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"))
				.setScale(0.6f),
			this, menu_selector(CircleToolPopup::on_info2)
		);
		info_btn->setPosition(ccp(60, 10) + ccp(42, -1.5));
		menu->addChild(info_btn);

		this->setTouchEnabled(true);

		return true;
	}

	void keyBackClicked() override {
		this->setTouchEnabled(false);
		this->removeFromParentAndCleanup(true);
	}

	void on_close(CCObject*) {
		this->keyBackClicked();
	}

	void update_labels() {
		auto objs = GameManager::sharedState()->getEditorLayer()->m_editorUI->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		const auto obj_count = amt * objs->count();
		m_label->setString(("Copies: " + std::to_string(amt) + "\nObjects: " + std::to_string(obj_count)).c_str());
	}

	void on_apply(CCObject*) {
		auto* editor = GameManager::sharedState()->getEditorLayer()->m_editorUI;
		auto objs = editor->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		if (objs && objs->count()) {
			const auto obj_count = objs->count() * amt;
			if (obj_count > 5000) {
				FLAlertLayer::create(this, "Warning",
					"This will create <cy>" + std::to_string(obj_count) + "</c> objects, are you sure?",
					"Cancel", "Ok"
				)->show();
			} else {
				perform();
			}
		}
	}

	void FLAlert_Clicked(FLAlertLayer*, bool btn2) override {
		if (btn2)
			perform();
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
		FLAlertLayer::create(this, "info", 
			"This will repeatedly copy-paste and rotate the selected objects,\n"
			"rotating by <cy>Step</c> degrees each time, until it gets to <cy>Arc</c> degrees.",
			"ok", nullptr, 300.f
		)->show();
	}

	void on_info2(CCObject*) {
		FLAlertLayer::create(this, "info",
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
