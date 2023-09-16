#include "nodes.hpp"
#include <array>
#include <charconv>
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
	try {
		return std::stoi(input_node->getString());
	} catch (const std::invalid_argument&) {
		return -1;
	}
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
	// TODO: not hardcode this
	std::array<char, 10> arr;
#ifndef __cpp_lib_to_chars
	// macos std::to_chars does not support floats at all, awesome
	const auto str = fmt::format("{}", value);
	if (str.size() >= 10) {
		input_node->setString("1");
	} else {
		input_node->setString(str);
	}
#else
	const auto result = std::to_chars(arr.data(), arr.data() + arr.size(), value, std::chars_format::fixed);
	if (result.ec == std::errc::value_too_large) {
		// TODO: have this be adjustable
		input_node->setString("1");
	} else {
		*result.ptr = 0;
		input_node->setString(arr.data());
	}
#endif
}

std::optional<float> FloatInputNode::get_value() {
	const auto str = input_node->getString();
#ifndef __cpp_lib_to_chars
	try {
		return std::stof(str);
	} catch (const std::invalid_argument&) {
		return {};
	} catch (const std::out_of_range&) {
		return {};
	}
#else
	float value = 0.f;
	const auto result = std::from_chars(str, str + std::strlen(str), value);
	if (result.ec == std::errc::invalid_argument)
		return {};
	return value;
#endif
}