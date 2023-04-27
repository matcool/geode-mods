#include <Geode/Geode.hpp>
#include <Geode/modify/ColorSelectPopup.hpp>
#include <Geode/modify/SetupPulsePopup.hpp>

using namespace geode::prelude;

inline std::string color_to_hex(ccColor3B color) {
	static constexpr auto digits = "0123456789ABCDEF";
	std::string output;
	output += digits[color.r >> 4 & 0xF];
	output += digits[color.r      & 0xF];
	output += digits[color.g >> 4 & 0xF];
	output += digits[color.g      & 0xF];
	output += digits[color.b >> 4 & 0xF];
	output += digits[color.b      & 0xF];
	return output;
}

class RGBColorInputWidget : public CCLayer, public TextInputDelegate {
	extension::CCControlColourPicker* parent;
	CCTextInputNode* red_input;
	CCTextInputNode* green_input;
	CCTextInputNode* blue_input;
	CCTextInputNode* hex_input;

	bool init(extension::CCControlColourPicker* parent) {
		if (!CCLayer::init()) return false;
		this->parent = parent;

		const ccColor3B placeholder_color = {100, 100, 100};

		constexpr float total_w = 115.f;
		constexpr float spacing = 4.f;
		constexpr float comp_width = (total_w - spacing * 2.f) / 3.f; // components (R G B) width
		constexpr float comp_height = 22.5f;
		constexpr float hex_height = 30.f;
		constexpr float hex_y = -hex_height - spacing / 2.f;
		constexpr float r_xpos = -comp_width - spacing;
		constexpr float b_xpos = -r_xpos;
		constexpr float bg_scale = 1.6f;
		constexpr GLubyte opacity = 100;

		red_input = CCTextInputNode::create(30.f, 20.f, "R", "bigFont.fnt");
		red_input->setAllowedChars("0123456789");
		red_input->m_maxLabelLength = 3;
		// red_input->setMaxLabelLength(3);
		red_input->setMaxLabelScale(0.6f);
		red_input->setLabelPlaceholderColor(placeholder_color);
		red_input->setLabelPlaceholderScale(0.5f);
		red_input->setPositionX(r_xpos);
		red_input->setDelegate(this);

		green_input = CCTextInputNode::create(30.f, 20.f, "G", "bigFont.fnt");
		green_input->setAllowedChars("0123456789");
		green_input->m_maxLabelLength =3;
		green_input->setMaxLabelScale(0.6f);
		green_input->setLabelPlaceholderColor(placeholder_color);
		green_input->setLabelPlaceholderScale(0.5f);
		green_input->setPositionX(0.f);
		green_input->setDelegate(this);
		
		blue_input = CCTextInputNode::create(30.f, 20.f, "B", "bigFont.fnt");
		blue_input->setAllowedChars("0123456789");
		blue_input->m_maxLabelLength  = 3;
		blue_input->setMaxLabelScale(0.6f);
		blue_input->setLabelPlaceholderColor(placeholder_color);
		blue_input->setLabelPlaceholderScale(0.5f);
		blue_input->setPositionX(b_xpos);
		blue_input->setDelegate(this);

		hex_input = CCTextInputNode::create(100.f, 20.f, "hex", "bigFont.fnt");
		hex_input->setAllowedChars("0123456789ABCDEFabcdef");
		hex_input->m_maxLabelLength = 6;
		hex_input->setMaxLabelScale(0.7f);
		hex_input->setLabelPlaceholderColor(placeholder_color);
		hex_input->setLabelPlaceholderScale(0.5f);
		hex_input->setPositionY(hex_y);
		hex_input->setDelegate(this);

		addChild(red_input);
		addChild(green_input);
		addChild(blue_input);
		addChild(hex_input);

		update_labels(true, true);

		auto bg = extension::CCScale9Sprite::create("square02_small.png");
		bg->setContentSize({total_w * bg_scale, hex_height * bg_scale});
		bg->setScale(1.f / bg_scale);
		bg->setOpacity(opacity);
		bg->setZOrder(-1);
		bg->setPositionY(hex_y);
		addChild(bg);

		bg = extension::CCScale9Sprite::create("square02_small.png");
		bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
		bg->setScale(1.f / bg_scale);
		bg->setOpacity(opacity);
		bg->setZOrder(-1);
		bg->setPositionX(r_xpos);
		addChild(bg);

		bg = extension::CCScale9Sprite::create("square02_small.png");
		bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
		bg->setScale(1.f / bg_scale);
		bg->setOpacity(opacity);
		bg->setZOrder(-1);
		addChild(bg);

		bg = extension::CCScale9Sprite::create("square02_small.png");
		bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
		bg->setScale(1.f / bg_scale);
		bg->setOpacity(opacity);
		bg->setZOrder(-1);
		bg->setPositionX(b_xpos);
		addChild(bg);

		return true;
	}

	bool ignore = false; // lmao this is such a hacky fix

	virtual void textChanged(CCTextInputNode* input) {
		if (ignore) return;
		if (input == hex_input) {
			std::string value(input->getString());
			ccColor3B color;
			if (value.empty()) return;
			int num_value;
			try {
				num_value = std::stoi(value, 0, 16);
			} catch (std::invalid_argument) {
				return;
			} catch (std::out_of_range) {
				return;
			}
			if (value.size() == 6) {
				auto r = static_cast<uint8_t>((num_value & 0xFF0000) >> 16);
				auto g = static_cast<uint8_t>((num_value & 0x00FF00) >> 8);
				auto b = static_cast<uint8_t>((num_value & 0x0000FF));
				color = {r, g, b};
			} else if (value.size() == 2) {
				auto number = static_cast<uint8_t>(num_value);
				color = {number, number, number};
			} else {
				return;
			}
			ignore = true;
			parent->setColorValue(color);
			ignore = false;
			update_labels(false, true);
		} else if (input == red_input || input == green_input || input == blue_input) {
			std::string value(input->getString());
			int num_value;
			try {
				num_value = value.empty() ? 0 : std::stoi(value);
			} catch (std::invalid_argument) {
				return;
			} catch (std::out_of_range) {
				return;
			}
			if (num_value > 255) {
				num_value = 255;
				input->setString("255");
			}
			auto num = static_cast<GLubyte>(num_value);
			auto color = parent->getColorValue();
			if (input == red_input)
				color.r = num;
			else if (input == green_input)
				color.g = num;
			else if (input == blue_input)
				color.b = num;
			ignore = true;
			parent->setColorValue(color);
			ignore = false;
			update_labels(true, false);
		}
	}

public:
	void update_labels(bool hex, bool rgb) {
		if (ignore) return;
		ignore = true;
		auto color = parent->getColorValue();
		if (hex) {
			hex_input->setString(color_to_hex(color).c_str());
		}
		if (rgb) {
			red_input->setString(std::to_string(color.r).c_str());
			green_input->setString(std::to_string(color.g).c_str());
			blue_input->setString(std::to_string(color.b).c_str());
		}
		ignore = false;
	}

	static auto create(extension::CCControlColourPicker* parent) {
		auto ptr = new (std::nothrow) RGBColorInputWidget();
		if (ptr && ptr->init(parent)) {
			ptr->autorelease();
		} else {
			delete ptr;
		}
		return ptr;
	}
};

class $modify(ColorSelectPopup) {
	RGBColorInputWidget* m_widget;

	bool init(EffectGameObject* trigger, CCArray* triggers, ColorAction* action) {
		if (!ColorSelectPopup::init(trigger, triggers, action)) return false;

		m_fields->m_widget = nullptr;
		auto widget = RGBColorInputWidget::create(m_colorPicker);
		if (!widget) return true;

		m_fields->m_widget = widget;
		auto center = CCDirector::sharedDirector()->getWinSize() / 2.f;
		if (m_isColorTrigger || trigger != nullptr) {
			widget->setPosition({center.width - 155.f, center.height + 29.f});
		} else {
			widget->setPosition({center.width + 127.f, center.height - 90.f});
		}
		m_mainLayer->addChild(widget);
		widget->setVisible(!m_copyColor);

		return true;
	}

	void colorValueChanged(cocos2d::ccColor3B color) {
		ColorSelectPopup::colorValueChanged(color);
		if (auto widget = m_fields->m_widget)
			widget->update_labels(true, true);
	}

	void updateCopyColorTextInputLabel() {
		ColorSelectPopup::updateCopyColorTextInputLabel();
		if (auto widget = m_fields->m_widget) {
			widget->setVisible(!m_copyColor);
			widget->update_labels(true, true);
		}
	}
};

class $modify(SetupPulsePopup) {
	RGBColorInputWidget* m_widget;

	bool init(EffectGameObject* trigger, CCArray* triggers) {
		if (!SetupPulsePopup::init(trigger, triggers)) return false;

		m_fields->m_widget = nullptr;
		auto widget = RGBColorInputWidget::create(m_colorPicker);
		if (!widget) return true;

		m_fields->m_widget = widget;
		auto center = CCDirector::sharedDirector()->getWinSize() / 2.f;

		m_colorPicker->setPositionX(m_colorPicker->getPositionX() + 3.7f);
		
		widget->setPosition({center.width - 132.f, center.height + 32.f});
		
		auto square_width = m_currentColorSpr->getScaledContentSize().width;
		auto x = widget->getPositionX() - square_width / 2.f;

		m_currentColorSpr->setPosition({x, center.height + 85.f});
		m_prevColorSpr->setPosition({x + square_width, center.height + 85.f});
		m_mainLayer->addChild(widget);
		widget->setVisible(m_pulseMode == 0);

		return true;
	}

	void colorValueChanged(cocos2d::ccColor3B color) {
		SetupPulsePopup::colorValueChanged(color);
		if (auto widget = m_fields->m_widget)
			widget->update_labels(true, true);
	}

	void onSelectPulseMode(CCObject* sender) {
		SetupPulsePopup::onSelectPulseMode(sender);
		if (auto widget = m_fields->m_widget) {
			widget->setVisible(m_pulseMode == 0);
			widget->update_labels(true, true);
		}
	}
};
