#include <Geode/Bindings.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <nodes.hpp>
#include <fmt/core.h>
#include <string_view>

using namespace geode::prelude;

enum class Position {
	Left,
	Right
};

class RunInfoWidget : public CCNode {
public:
	CCLabelBMFont* m_status_label = nullptr;
	CCLabelBMFont* m_info_label = nullptr;
	CCSprite* m_icon_sprite = nullptr;
	Position m_last_pos;
	bool m_was_practice = true;

	static RunInfoWidget* create(Position pos) {
		auto* ret = new (std::nothrow) RunInfoWidget;
		if (ret && ret->init(pos)) {
			ret->autorelease();
			return ret;
		} else {
			delete ret;
			return nullptr;
		}
	}

	bool init(Position pos) {
		if (!CCNode::init()) return false;
		m_status_label = make_factory(CCLabelBMFont::create("Practice", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.5f)
			.addTo(this)
			.end();

		m_info_label = make_factory(CCLabelBMFont::create("From 0%", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.4f)
			.addTo(this)
			.end();

		this->reset_icon("checkpoint_01_001.png");

		this->update_position(pos);

		float height = m_status_label->getScaledContentSize().height + m_info_label->getScaledContentSize().height;
		float width = m_status_label->getScaledContentSize().width;
		this->setContentSize({width, height});

		return true;
	}

	void reset_icon(const char* str) {
		if (m_icon_sprite) m_icon_sprite->removeFromParent();
		m_icon_sprite = make_factory(CCSprite::createWithSpriteFrameName(str))
			.setOpacity(64)
			.addTo(this)
			.with([&](auto* sprite) {
				sprite->setScale(m_status_label->getScaledContentSize().height / sprite->getContentSize().height);
			})
			.end();
	}

	void update_position(Position pos) {
		float y = m_info_label->getScaledContentSize().height;
		m_icon_sprite->setPositionY(y - 1.f);
		m_status_label->setPositionY(y);
		m_info_label->setPositionY(0.f);

		const bool left = pos == Position::Left;
		const float h_flip = left ? 1.f : -1.f;
		const auto anchor = ccp(left ? 0.f : 1.f, 0.f);
		
		m_icon_sprite->setAnchorPoint(anchor);
		m_status_label->setAnchorPoint(anchor);
		m_info_label->setAnchorPoint(anchor);

		float x = 5.f;

		m_icon_sprite->setPositionX(h_flip * x);
		m_status_label->setPositionX(h_flip * (x + m_icon_sprite->getScaledContentSize().width + 2.f));
		m_info_label->setPositionX(h_flip * x);

		m_last_pos = pos;
	}

	void update_labels(PlayLayer* layer, float x) {
		if (layer->m_isPracticeMode) {
			m_status_label->setString("Practice");
			if (!m_was_practice)
				this->reset_icon("checkpoint_01_001.png");
			m_was_practice = true;
		} else if (layer->m_isTestMode) {
			m_status_label->setString("Testmode");
			if (m_was_practice)
				this->reset_icon("edit_eStartPosBtn_001.png");
			m_was_practice = false;
		}

		int percent = static_cast<int>(x / layer->m_levelLength * 100.f);
		m_info_label->setString(fmt::format("From {}%", percent).c_str());

		this->update_position(m_last_pos);
	}
};


class $modify(PlayLayer) {
	RunInfoWidget* m_widget = nullptr;
	float m_initial_x;

	bool init(GJGameLevel* level) {
		if (!PlayLayer::init(level)) return false;

		// removes the testmode label gd creates
		if (this->getChildrenCount()) {
			CCArrayExt<CCNode*> children = this->getChildren();
			for (auto* child : children) {
				using namespace std::literals::string_view_literals;
				if (auto* label = typeinfo_cast<CCLabelBMFont*>(child); label && label->getString() == "Testmode"sv) {
					label->setVisible(false);
					break;
				}
			}
		}

		const auto win_size = CCDirector::sharedDirector()->getWinSize();

		m_fields->m_widget = make_factory(RunInfoWidget::create(Position::Left))
			.setAnchorPoint(ccp(0.f, 1.f))
			.setPosition(0.f, win_size.height - 2.f)
			.setZOrder(999)
			.addTo(this)
			.end();

		// FIXME: switch to an enum type of setting.. whenever thats available (or make one myself)
		this->update_position(
			Mod::get()->getSettingValue<bool>("position-top"),
			Mod::get()->getSettingValue<bool>("position-left")
		);

		this->update_labels();

		return true;
	}

	void update_position(bool top, bool left) {
		auto* widget = m_fields->m_widget;

		widget->update_position(left ? Position::Left : Position::Right);

		const auto win_size = CCDirector::sharedDirector()->getWinSize();

		if (left) {
			widget->setPositionX(0.f);
		} else {
			widget->setPositionX(win_size.width);
		}

		if (top) {
			widget->setAnchorPoint(ccp(0.f, 1.f));
			widget->setPositionY(win_size.height - 2.f);
		} else {
			widget->setAnchorPoint(ccp(0.f, 0.f));
			widget->setPositionY(4.f);
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		m_fields->m_initial_x = m_player1->getPosition().x;
		this->update_labels();
	}

	void togglePracticeMode(bool toggle) {
		PlayLayer::togglePracticeMode(toggle);
		this->update_labels();
	}

	void update_labels() {
		if (m_fields->m_widget) {
			m_fields->m_widget->update_labels(this, m_fields->m_initial_x);
			m_fields->m_widget->setVisible(m_isPracticeMode || m_isTestMode);
		}
	}
};