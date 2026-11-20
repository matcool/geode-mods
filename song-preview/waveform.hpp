#pragma once

#include <Geode/Geode.hpp>
#include <span>

using namespace geode::prelude;

class WaveformNode : public CCNode {
public:
	Ref<CCTexture2D> m_texture;
	CCSprite* m_sprite;

	static WaveformNode* create(CCSize size) {
		auto* node = new WaveformNode();
		node->init(size);
		node->autorelease();
		return node;
	}

	bool init(CCSize size);
	void setPercent(float);

	void drawSamples(std::span<const float> samples);
};