#include <Geode/Bindings.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <nodes.hpp>
#include <fmt/core.h>
#include <string_view>

USE_GEODE_NAMESPACE();

enum class Position {
	Left,
	Right
};

class RunInfoWidget : public CCNode {
public:
	CCLabelBMFont* m_statusLabel = nullptr;
	CCLabelBMFont* m_infoLabel = nullptr;
	CCSprite* m_iconSprite = nullptr;

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
		
		m_statusLabel = make_factory(CCLabelBMFont::create("Practice", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.5f)
			.addTo(this)
			.end();

		m_infoLabel = make_factory(CCLabelBMFont::create("From 0%", "bigFont.fnt"))
			.setOpacity(64)
			.setScale(0.4f)
			.addTo(this)
			.end();

		m_iconSprite = make_factory(CCSprite::createWithSpriteFrameName("checkpoint_01_001.png"))
			.setOpacity(64)
			.addTo(this)
			.with([&](auto* sprite) {
				sprite->setScale(m_statusLabel->getScaledContentSize().height / sprite->getContentSize().height);
			})
			.end();

		this->update_position(pos);

		float height = m_statusLabel->getScaledContentSize().height + m_infoLabel->getScaledContentSize().height;
		float width = m_statusLabel->getScaledContentSize().width;
		this->setContentSize({width, height});

		return true;
	}

	void update_position(Position pos) {
		float y = m_infoLabel->getScaledContentSize().height;
		m_iconSprite->setPositionY(y - 1.f);
		m_statusLabel->setPositionY(y);
		m_infoLabel->setPositionY(0.f);

		const bool left = pos == Position::Left;
		const float h_flip = left ? 1.f : -1.f;
		const auto anchor = ccp(left ? 0.f : 1.f, 0.f);
		
		m_iconSprite->setAnchorPoint(anchor);
		m_statusLabel->setAnchorPoint(anchor);
		m_infoLabel->setAnchorPoint(anchor);

		float x = 5.f;

		m_iconSprite->setPositionX(h_flip * x);
		m_statusLabel->setPositionX(h_flip * (x + m_iconSprite->getScaledContentSize().width + 2.f));
		m_infoLabel->setPositionX(h_flip * x);
	}

	void update_labels(PlayLayer* layer, float x) {
		if (layer->m_isPracticeMode) {
			m_statusLabel->setString("Practice");
		} else if (layer->m_isTestMode) {
			m_statusLabel->setString("Testmode");
		}

		int percent = static_cast<int>(x / layer->m_levelLength * 100.f);
		m_infoLabel->setString(fmt::format("From {}%", percent).c_str());
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
					label->removeFromParentAndCleanup(true);
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
		if (m_fields->m_widget)
			m_fields->m_widget->update_labels(this, m_fields->m_initial_x);
	}

	void togglePracticeMode(bool toggle) {
		PlayLayer::togglePracticeMode(toggle);
		m_fields->m_widget->update_labels(this, m_fields->m_initial_x);
	}
};