#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <nodes.hpp>
#include <fmt/core.h>
#include <string_view>

using namespace geode::prelude;

class RunInfoWidget : public CCNodeRGBA {
public:
	CCLabelBMFont* m_status_label = nullptr;
	CCLabelBMFont* m_info_label = nullptr;
	CCSprite* m_icon_sprite = nullptr;
	CCNodeRGBA* m_top_layout = nullptr;
	bool m_was_practice = true;

	static RunInfoWidget* create(PlayLayer* layer, bool left) {
		auto* ret = new (std::nothrow) RunInfoWidget;
		if (ret && ret->init(layer, left)) {
			ret->autorelease();
			return ret;
		} else {
			delete ret;
			return nullptr;
		}
	}

	bool init(PlayLayer* layer, bool left) {
		if (!CCNodeRGBA::init()) return false;
		this->setCascadeColorEnabled(true);
		this->setCascadeOpacityEnabled(true);

		m_top_layout = CCNodeRGBA::create();
		m_top_layout->setCascadeColorEnabled(true);
		m_top_layout->setCascadeOpacityEnabled(true);
		this->addChild(m_top_layout);
		m_top_layout->setLayout(
			AxisLayout::create(Axis::Row)
			->setAutoScale(false)
			->setGap(2.f)
			->setAutoGrowAxis(true)
			->setGrowCrossAxis(false)
			->setCrossAxisOverflow(true)
			->setAxisAlignment(AxisAlignment::Start)
		);
		m_top_layout->getLayout()->ignoreInvisibleChildren(true);
		m_top_layout->setID("top-layout"_spr);

		m_status_label = make_factory(CCLabelBMFont::create("Practice", "bigFont.fnt"))
			.setScale(0.5f)
			.setID("mode-label"_spr)
			.addTo(m_top_layout)
			.setZOrder(1)
			.end();

		m_info_label = make_factory(CCLabelBMFont::create("From 0%", "bigFont.fnt"))
			.setScale(0.4f)
			.setID("from-label"_spr)
			.addTo(this)
			.end();

		this->setID("RunInfoWidget"_spr);
		this->reset_icon("checkpoint_01_001.png");

		this->setLayout(
			AxisLayout::create(Axis::Column)
			->setAutoScale(false)
			->setGap(0.f)
			->setAxisReverse(true)
			->setAutoGrowAxis(true)
			->setGrowCrossAxis(false)
			->setCrossAxisOverflow(true)
			->setCrossAxisAlignment(AxisAlignment::Start)
			->setCrossAxisLineAlignment(AxisAlignment::Start)
		);
		this->getLayout()->ignoreInvisibleChildren(true);

		this->update_position(layer, left);

		return true;
	}

	void reset_icon(const char* str) {
		if (m_icon_sprite) m_icon_sprite->removeFromParent();
		m_icon_sprite = make_factory(CCSprite::createWithSpriteFrameName(str))
			.setID("icon-sprite"_spr)
			.addTo(m_top_layout)
			.with([&](auto* sprite) {
				sprite->setScale(m_status_label->getScaledContentSize().height / sprite->getContentSize().height);
			})
			.end();
		// make cocos cascade the opacity lol
		this->setOpacity(this->getOpacity());
	}

	void update_position(PlayLayer* layer, bool left) {
		bool show_icon = Mod::get()->getSettingValue<bool>("show-icon");
		if (!Mod::get()->getSettingValue<bool>("use-start-pos") && !m_was_practice)
			show_icon = false;
		const bool show_status = Mod::get()->getSettingValue<bool>("show-mode");
		const bool is_platformer = layer->m_level->isPlatformer();
		// From x% is useless in platformer mode, force it off
		const bool show_info = !is_platformer && Mod::get()->getSettingValue<bool>("show-from");

		static_cast<AxisLayout*>(this->getLayout())->setCrossAxisLineAlignment(left ? AxisAlignment::Start : AxisAlignment::End);
		static_cast<AxisLayout*>(m_top_layout->getLayout())->setAxisReverse(!left);

		m_icon_sprite->setVisible(show_icon);
		m_status_label->setVisible(show_status);
		m_info_label->setVisible(show_info);

		m_top_layout->setVisible(m_icon_sprite->isVisible() || m_status_label->isVisible());

		if (Mod::get()->getSettingValue<bool>("same-line") && !m_status_label->isVisible()) {
			static_cast<AxisLayout*>(this->getLayout())
				->setAxis(Axis::Row)
				->setCrossAxisLineAlignment(AxisAlignment::Center)
				->setAxisReverse(!left)
				->setGap(2.f);
		} else {
			static_cast<AxisLayout*>(this->getLayout())
				->setAxis(Axis::Column)
				->setAxisReverse(true)
				->setGap(0.f);
		}

		m_top_layout->updateLayout();
		this->updateLayout();
	}

	void update_labels(PlayLayer* layer, float percent) {
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

		auto decimal = Mod::get()->getSettingValue<int64_t>("decimal-places");

		m_info_label->setString(fmt::format("From {0:.{1}f}%", percent, decimal).c_str());

		m_top_layout->updateLayout();
		this->updateLayout();
	}
};


class $modify(PlayLayer) {
	struct Fields {
		RunInfoWidget* m_widget = nullptr;
		float m_initial_percent = 0;
		bool m_show_in_percentage = false;
	};

	bool init(GJGameLevel* level, bool unk1, bool unk2) {
		if (!PlayLayer::init(level, unk1, unk2)) return false;

		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

		m_fields->m_show_in_percentage = Mod::get()->getSettingValue<bool>("show-in-percentage");

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

		m_fields->m_widget = make_factory(RunInfoWidget::create(this, true))
			.setAnchorPoint(ccp(0.f, 1.f))
			.setPosition(0.f, win_size.height - 2.f)
			.setZOrder(999)
			.addTo(this)
			.end();

		m_fields->m_widget->setOpacity(Mod::get()->getSettingValue<int>("opacity"));
		m_fields->m_widget->setScale(Mod::get()->getSettingValue<float>("scale"));
		m_fields->m_widget->setColor(Mod::get()->getSettingValue<ccColor3B>("color"));

		this->update_labels();
		this->update_position();

		return true;
	}

	void update_labels() {
		if (!m_fields->m_widget || !Mod::get()->getSettingValue<bool>("enabled")) return;
		m_fields->m_widget->update_labels(this, m_fields->m_initial_percent);
		m_fields->m_widget->setVisible(m_isPracticeMode || m_isTestMode);
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

		widget->update_position(this, left);

		const auto win_size = CCDirector::sharedDirector()->getWinSize();

		if (left) {
			widget->setPositionX(5.f);
		} else {
			widget->setPositionX(win_size.width - 5.f);
		}

		if (top) {
			widget->setAnchorPoint(ccp(left ? 0.f : 1.f, 1.f));
			widget->setPositionY(win_size.height - 2.f);
		} else {
			widget->setAnchorPoint(ccp(left ? 0.f : 1.f, 0.f));
			widget->setPositionY(4.f);
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		m_fields->m_initial_percent = this->getCurrentPercent();
		this->update_labels();
		this->update_position();
	}

	void togglePracticeMode(bool toggle) {
		PlayLayer::togglePracticeMode(toggle);
		this->update_labels();
		this->update_position();
	}

	void updateProgressbar() {
		PlayLayer::updateProgressbar();
		if (m_isPlatformer || !(m_isPracticeMode || m_isTestMode))
			return;
		if (m_fields->m_show_in_percentage) {
			auto decimal = Mod::get()->getSettingValue<int64_t>("decimal-places");
			auto from = m_fields->m_initial_percent;
			auto to = this->getCurrentPercent();
			m_percentageLabel->setString(fmt::format("{1:.{0}f}-{2:.{0}f}%", decimal, from, to).c_str());
		}
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
	if (container.contains("use-decimal")) {
		auto useDecimals = container["use-decimal"].asBool().unwrapOr(false);
		log::debug("Migrating from old settings: use-decimal={}", useDecimals);

		container.erase("use-decimal");

		Mod::get()->setSettingValue<int64_t>("decimal-places", useDecimals ? 2 : 0);
	}
}
