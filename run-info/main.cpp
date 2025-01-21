#include <Geode/Geode.hpp>
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
	bool m_was_practice = true;

	static RunInfoWidget* create(PlayLayer* layer, Position pos) {
		auto* ret = new (std::nothrow) RunInfoWidget;
		if (ret && ret->init(layer, pos)) {
			ret->autorelease();
			return ret;
		} else {
			delete ret;
			return nullptr;
		}
	}

	bool init(PlayLayer* layer, Position pos) {
		if (!CCNode::init()) return false;
		m_status_label = make_factory(CCLabelBMFont::create("Practice", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.5f)
			.setID("mode-label"_spr)
			.addTo(this)
			.end();

		m_info_label = make_factory(CCLabelBMFont::create("From 0%", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.4f)
			.setID("from-label"_spr)
			.addTo(this)
			.end();

		this->setID("RunInfoWidget"_spr);
		this->reset_icon("checkpoint_01_001.png");

		this->update_position(layer, pos);

		float height = m_status_label->getScaledContentSize().height + m_info_label->getScaledContentSize().height;
		float width = m_status_label->getScaledContentSize().width;
		this->setContentSize({width, height});

		return true;
	}

	void reset_icon(const char* str) {
		if (m_icon_sprite) m_icon_sprite->removeFromParent();
		m_icon_sprite = make_factory(CCSprite::createWithSpriteFrameName(str))
			.setOpacity(64)
			.setID("icon-sprite"_spr)
			.addTo(this)
			.with([&](auto* sprite) {
				sprite->setScale(m_status_label->getScaledContentSize().height / sprite->getContentSize().height);
			})
			.end();
	}

	void update_position(PlayLayer* layer, Position pos) {
		bool show_icon = Mod::get()->getSettingValue<bool>("show-icon");
		if (!Mod::get()->getSettingValue<bool>("use-start-pos") && !m_was_practice)
			show_icon = false;
		const bool show_status = Mod::get()->getSettingValue<bool>("show-mode");
		const bool is_platformer = layer->m_level->isPlatformer();
		// From x% is useless in platformer mode, force it off
		const bool show_info = !is_platformer && Mod::get()->getSettingValue<bool>("show-from");

		const float y = show_info ? m_info_label->getScaledContentSize().height : 0.f;
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
		if (show_icon)
			m_status_label->setPositionX(h_flip * (x + m_icon_sprite->getScaledContentSize().width + 2.f));
		else
			m_status_label->setPositionX(h_flip * x);
		m_info_label->setPositionX(h_flip * x);

		m_icon_sprite->setVisible(show_icon);
		m_status_label->setVisible(show_status);
		m_info_label->setVisible(show_info);

		float height = 0.f;
		if (show_info)
			height += m_info_label->getScaledContentSize().height;
		if (show_status || show_icon)
			height += m_status_label->getScaledContentSize().height;
		this->setContentSize({this->getContentSize().width, height});
	}

	void update_labels(PlayLayer* layer, int percent) {
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

		m_info_label->setString(fmt::format("From {}%", percent).c_str());
	}
};


class $modify(PlayLayer) {
	struct Fields {
		RunInfoWidget* m_widget = nullptr;
		// TODO: once we recreate getCurrentPercent maybe switch to float,
		// and add option for decimals
		int m_initial_percent = 0;
	};

	bool init(GJGameLevel* level, bool unk1, bool unk2) {
		if (!PlayLayer::init(level, unk1, unk2)) return false;

		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

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

		m_fields->m_widget = make_factory(RunInfoWidget::create(this, Position::Left))
			.setAnchorPoint(ccp(0.f, 1.f))
			.setPosition(0.f, win_size.height - 2.f)
			.setZOrder(999)
			.addTo(this)
			.end();

		this->update_labels();
		this->update_position();

		return true;
	}

	void update_position() {
		auto const position = Mod::get()->getSettingValue<std::string>("position");
		this->update_position(
			position == "Top Left" || position == "Top Right",
			position == "Top Left" || position == "Bottom Left"
		);
	}

	void update_position(bool top, bool left) {
		if (!m_fields->m_widget || !Mod::get()->getSettingValue<bool>("enabled")) return;

		auto* widget = m_fields->m_widget;

		widget->update_position(this, left ? Position::Left : Position::Right);

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
		m_fields->m_initial_percent = this->getCurrentPercentInt();
		this->update_labels();
		this->update_position();
	}

	void togglePracticeMode(bool toggle) {
		PlayLayer::togglePracticeMode(toggle);
		this->update_labels();
		this->update_position();
	}

	void update_labels() {
		if (!m_fields->m_widget || !Mod::get()->getSettingValue<bool>("enabled")) return;
		m_fields->m_widget->update_labels(this, m_fields->m_initial_percent);
		m_fields->m_widget->setVisible(m_isPracticeMode || m_isTestMode);
	}
};

$on_mod(Loaded) {
	// migrate old settings to "position"
	auto& container = Mod::get()->getSavedSettingsData();
	if (container.contains("position-top") || container.contains("position-left")) {
		auto top = container["position-top"].asBool().unwrapOr(true);
		auto left = container["position-left"].asBool().unwrapOr(true);
		log::debug("Migrating from old settings: top={} left={}", top, left);

		container.erase("position-top");
		container.erase("position-left");

		Mod::get()->setSettingValue<std::string>("position", top ? (left ? "Top Left" : "Top Right") : (left ? "Bottom Left" : "Bottom Right"));
	}
}