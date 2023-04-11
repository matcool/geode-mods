#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <numbers>
#include <box2d/box2d.h>
#include <memory>
#include <fmt/core.h>

using namespace geode::prelude;

static constexpr float ratio = 64.f;

class NodeBox2D {
	CCNode* m_node;
	b2Body* m_body;
public:
	NodeBox2D(CCNode* node, b2Body* body) : m_node(node), m_body(body) {}

	void update() {
		const auto pos2d = m_body->GetPosition();
		const auto pos = m_node->getParent()->convertToNodeSpace(ccp(pos2d.x * ratio, pos2d.y * ratio));
		m_node->setPosition(pos);

		const auto rot = m_body->GetAngle();
		m_node->setRotation(CC_RADIANS_TO_DEGREES(rot));
	}
};

NodeBox2D add_sprite(b2World* world, CCNode* node) {
	const auto pos = node->getParent()->convertToWorldSpace(node->getPosition());

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(pos.x / ratio, pos.y / ratio);
	bodyDef.angle = CC_DEGREES_TO_RADIANS(node->getRotation());

	b2Body* body = world->CreateBody(&bodyDef);

	const auto size = node->getScaledContentSize();

	b2PolygonShape dynamicBox;
	dynamicBox.SetAsBox(size.width / 2.f / ratio, size.height / 2.f / ratio);
	
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	body->CreateFixture(&fixtureDef);

	return NodeBox2D(node, body);
}

template <class T, class U>
bool is(U ptr) { return typeinfo_cast<T>(ptr) != nullptr; }

template <class... Ts>
bool is_one_of(CCNode* ptr) {
	return (is<Ts*>(ptr) || ...);
}

b2World* world;
std::unordered_map<CCNode*, NodeBox2D> bodies;
class $modify(MyDirector, CCDirector) {
	void willSwitchToScene(CCScene* scene) {
		CCDirector::willSwitchToScene(scene);
		this->setup_world(scene);
	}

	void setup_world(CCScene* scene) {
		bodies.clear();
		if (!world) delete world;

		world = new b2World(b2Vec2(0.f, -5.f));

		b2BodyDef groundBodyDef;
		groundBodyDef.position.Set(0, -10.f);

		b2Body* groundBody = world->CreateBody(&groundBodyDef);

		b2PolygonShape groundBox;
		groundBox.SetAsBox(50.0f, 10.0f);

		groundBody->CreateFixture(&groundBox, 0.0f);

		this->getScheduler()->scheduleSelector(schedule_selector(MyDirector::do_thing), this, 0, 0, 0.5f, false);
		scene->schedule(schedule_selector(MyDirector::update_physics));
	}

	void do_thing(float) {
		auto* scene = this->getRunningScene();
		auto* children = scene->getChildren();
		if (children) {
			this->recurse_nodes(static_cast<CCNode*>(children->objectAtIndex(0)));
		}
	}

	void recurse_nodes(CCNode* node) {
		// log::info("recursing through {}", node);
		if (!node->getChildren() || !node->isVisible()) return;
		CCArrayExt<CCNode*> arr(node->getChildren());
		const auto win_size = this->getWinSize();
		for (auto child : arr) {
			if (is_one_of<CCMenuItemSpriteExtra, CCLabelBMFont, CCSprite, CCScale9Sprite>(child)) {
				auto size = child->getScaledContentSize();
				if (size.width > win_size.width / 2.f && size.height > win_size.height / 2.f)
					continue;
				bodies.insert({child, add_sprite(world, child)});
			// } else if (typeinfo_cast<CCMenu*>(child)) {
			} else {
				recurse_nodes(child);
			}
		}
	}

	void update_physics(float dt) {
		world->Step(1.f / 60.f, 8, 3);
		for (auto& [_, node] : bodies) {
			node.update();
		}
	}
};
