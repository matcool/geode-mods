#pragma once
#include <Geode/Bindings.hpp>
#include <functional>
#include <optional>

#define GEN_CREATE(class_name) template <typename... Args> \
static auto create(Args... args) { \
	auto node = new (std::nothrow) class_name; \
	if (node && node->init(args...)) \
		node->autorelease(); \
	else \
		CC_SAFE_DELETE(node); \
	return node; \
}

class TextInputNode : public cocos2d::CCNode, public TextInputDelegate {
public:
	CCTextInputNode* input_node;
	cocos2d::extension::CCScale9Sprite* background;
	std::function<void(TextInputNode&)> callback = [](auto&){};

	GEN_CREATE(TextInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");

	virtual void textChanged(CCTextInputNode*);

	~TextInputNode() {
		if (input_node) {
			input_node->onClickTrackNode(false);
			input_node->setDelegate(nullptr);
		}
	}

	void set_value(const std::string& value);
	std::string get_value();
};

class NumberInputNode : public TextInputNode {
public:
	std::function<void(NumberInputNode&)> callback = [](auto&){};

	GEN_CREATE(NumberInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");
	virtual void textChanged(CCTextInputNode*);

	void set_value(int value);
	int get_value();
};

class FloatInputNode : public TextInputNode {
public:
	std::function<void(FloatInputNode&)> callback = [](auto&){};
	
	GEN_CREATE(FloatInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");
	virtual void textChanged(CCTextInputNode*);

	void set_value(float value);
	std::optional<float> get_value();
};

template <typename T>
class NodeFactory {
public:
	static auto& start(T* node) { return *reinterpret_cast<NodeFactory*>(node); }
	
	template <typename... Args>
	static auto& start(Args... args) { return *reinterpret_cast<NodeFactory*>(T::create(args...)); }

	NodeFactory(const NodeFactory&) = delete;
	NodeFactory(NodeFactory&&) = delete;
	
	T* end() { return reinterpret_cast<T*>(this); }
	operator T*() { return end(); }
	operator cocos2d::CCNode*() { return end(); }

	inline auto& setPosition(const cocos2d::CCPoint p) { end()->setPosition(p); return *this; }
	inline auto& setPosition(float x, float y) { return this->setPosition(ccp(x, y)); }
	inline auto& setScale(float x, float y) { return this->setScaleX(x).setScaleY(y); }
	inline auto& setScale(float x) { end()->setScale(x); return *this; }

	#define GEN_FACTORY_FUNC(name) template <typename... Args> \
	inline auto& name(Args... args) { \
		end()->name(args...); \
		return *this; \
	}

	// GEN_FACTORY_FUNC(setPosition)
	// GEN_FACTORY_FUNC(setScale)
	GEN_FACTORY_FUNC(setScaleX)
	GEN_FACTORY_FUNC(setScaleY)
	GEN_FACTORY_FUNC(setContentSize)
	GEN_FACTORY_FUNC(setOpacity)
	GEN_FACTORY_FUNC(setZOrder)
	GEN_FACTORY_FUNC(setAnchorPoint)
	GEN_FACTORY_FUNC(addChild)
	GEN_FACTORY_FUNC(setTag)
	GEN_FACTORY_FUNC(setAlignment)
	GEN_FACTORY_FUNC(setColor)
	GEN_FACTORY_FUNC(limitLabelWidth)
	GEN_FACTORY_FUNC(setUserObject)
	GEN_FACTORY_FUNC(setVisible)
	GEN_FACTORY_FUNC(setID)

	#undef GEN_FACTORY_FUNC

	template <class Func>
	auto& with(Func&& function) {
		function(end());
		return *this;
	}

	auto& addTo(cocos2d::CCNode* node) {
		node->addChild(end());
		return *this;
	}
};

template <class T>
auto& make_factory(T* node) {
	return NodeFactory<T>::start(node);
}