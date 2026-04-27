#include "nodes.hpp"
#include <fmt/core.h>

using namespace cocos2d;

bool TextInputNode::init(CCSize size, float scale, const std::string& font) {
	if (!CCNode::init()) return false;

	input_node = CCTextInputNode::create(size.width, size.height, "", font.c_str());
	input_node->setDelegate(this);
	addChild(input_node);

	background = extension::CCScale9Sprite::create("square02_small.png");
	background->setContentSize(size * scale);
	background->setScale(1.f / scale);
	background->setOpacity(100);
	background->setZOrder(-1);
	addChild(background);

	return true;
}

void TextInputNode::textChanged(CCTextInputNode*) {
	callback(*this);
}

void TextInputNode::set_value(const std::string& value) {
	input_node->setString(value.c_str());
}

std::string TextInputNode::get_value() {
	return input_node->getString();
}


bool NumberInputNode::init(CCSize size, float scale, const std::string& font) {
	if (!TextInputNode::init(size, scale, font)) return false;
	input_node->setAllowedChars("0123456789");
	return true;
}

void NumberInputNode::textChanged(CCTextInputNode*) {
	callback(*this);
}

void NumberInputNode::set_value(int value) {
	input_node->setString(std::to_string(value).c_str());
}

int NumberInputNode::get_value() {
	return geode::utils::numFromString<int>(input_node->getString()).unwrapOr(-1);
}


bool FloatInputNode::init(CCSize size, float scale, const std::string& font) {
	if (!TextInputNode::init(size, scale, font)) return false;
	input_node->setAllowedChars("0123456789.");
	return true;
}

void FloatInputNode::textChanged(CCTextInputNode*) {
	callback(*this);
}

void FloatInputNode::set_value(float value) {
	input_node->setString(fmt::format("{}", value));
}

std::optional<float> parse_float(const std::string_view str) {
	return geode::utils::numFromString<float>(str).ok();
}

std::optional<float> FloatInputNode::get_value() {
	const auto str = input_node->getString();
	return parse_float(str);
}