#include "RenderTexture.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// #include <CCGL.h>
#include <thread>

void copy_screenshot(std::unique_ptr<uint8_t[]> data, const CCSize& size, int startX = 0, int startY = 0, int endX = 0, int endY = 0) {
	const auto width = static_cast<int>(size.width);
	const auto height = static_cast<int>(size.height);
	if (endX == 0) endX = width;
	if (endY == 0) endY = height;
	const auto newWidth = endX - startX;
	const auto newHeight = endY - startY;
	std::thread([=, data = std::move(data)]() {
		auto newData = std::make_unique<uint8_t[]>(newWidth * newHeight * 4);
		for (int y = startY; y < endY; ++y) {
			for (int x = startX; x < endX; ++x) {
				auto i = ((height - y - 1) * width + x) * 4;
				auto newIndex = ((y - startY) * newWidth + (x - startX)) * 4;
				// RGBA -> BGRA
				newData[newIndex + 0] = data[i + 2];
				newData[newIndex + 1] = data[i + 1];
				newData[newIndex + 2] = data[i + 0];
				newData[newIndex + 3] = data[i + 3];
			}
		}

		auto bitmap = CreateBitmap(newWidth, newHeight, 1, 32, newData.get());

		if (OpenClipboard(NULL))
			if (EmptyClipboard()) {
				SetClipboardData(CF_BITMAP, bitmap);
				CloseClipboard();
			}
	}).detach();
}

class ImageAreaSelectLayer : public FLAlertLayer {
	std::unique_ptr<uint8_t[]> m_data;
	CCSize m_texture_size;
	CCDrawNode* m_stencil;
	CCPoint m_start_pos, m_end_pos;
	CCRect m_sprite_rect;
	bool init(CCTexture2D* texture2d) {
		if (!initWithColor({0, 0, 0, 100})) return false;

		m_mainLayer = CCLayer::create();
		addChild(m_mainLayer);

		m_texture_size = texture2d->getContentSizeInPixels();

		const auto& winSize = CCDirector::sharedDirector()->getWinSize();

		auto sprite = CCSprite::createWithTexture(texture2d);
		sprite->setPosition(winSize / 2);
		sprite->setScale(winSize.width / sprite->getContentWidth() * 0.9f);
		sprite->setFlipY(true);
		m_mainLayer->addChild(sprite);

		{
			const auto size = sprite->getScaledContentSize();
			const auto pos = sprite->getPosition() - size / 2;
			m_sprite_rect = CCRect(pos.x, pos.y, size.width, size.height);
			m_start_pos = pos;
			m_end_pos = pos + size;
		}

		this->setKeypadEnabled(true);
		this->setTouchEnabled(true);

		auto overlay = CCSprite::create("square.png");
		overlay->setColor({0, 0, 0});
		overlay->setOpacity(200);
		overlay->setScaleX(sprite->getScaledContentSize().width / overlay->getContentSize().width);
		overlay->setScaleY(sprite->getScaledContentSize().height / overlay->getContentSize().height);
		overlay->setPosition(winSize / 2);

		m_stencil = CCDrawNode::create();
		update_stencil();

		auto clip = CCClippingNode::create(m_stencil);
		clip->addChild(overlay);
		clip->setAlphaThreshold(0.0);
		clip->setInverted(true);
		m_mainLayer->addChild(clip);
		
		return true;
	}

	void update_stencil() {
		m_stencil->clear();
		CCPoint verts[4] = {
			ccp(m_start_pos.x, m_start_pos.y),
			ccp(m_start_pos.x, m_end_pos.y),
			ccp(m_end_pos.x, m_end_pos.y),
			ccp(m_end_pos.x, m_start_pos.y)
		};
		m_stencil->drawPolygon(verts, 4, {1, 1, 1, 1}, 0, {0, 0, 0, 0});
	}

	virtual void ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent) {
		FLAlertLayer::ccTouchMoved(pTouch, pEvent);
		m_start_pos = pTouch->getStartLocation();
		m_end_pos = pTouch->getLocation();
		update_stencil();
	}

	virtual void keyDown(enumKeyCodes key) {
		FLAlertLayer::keyDown(key);
		if (key == enumKeyCodes::KEY_Enter) {
			auto xf = (m_start_pos.x - m_sprite_rect.origin.x) / m_sprite_rect.size.width;
			auto yf = (m_start_pos.y - m_sprite_rect.origin.y) / m_sprite_rect.size.height;
			auto af = (m_end_pos.x - m_sprite_rect.origin.x) / m_sprite_rect.size.width;
			auto bf = (m_end_pos.y - m_sprite_rect.origin.y) / m_sprite_rect.size.height;
			if (xf > af) std::swap(xf, af);
			if (bf > yf) std::swap(yf, bf);
			const auto width = m_texture_size.width;
			const auto height = m_texture_size.height;
			unsigned x = static_cast<unsigned>(std::clamp(xf * width, 0.f, width));
			unsigned y = static_cast<unsigned>(std::clamp((1.f - yf) * height, 0.f, height));
			unsigned a = static_cast<unsigned>(std::clamp(af * width, 0.f, width));
			unsigned b = static_cast<unsigned>(std::clamp((1.f - bf) * height, 0.f, height));
			copy_screenshot(std::move(m_data), m_texture_size, x, y, a, b);
			this->keyBackClicked();
		}
	}

public:
	static auto create(CCTexture2D* tex, std::unique_ptr<uint8_t[]> data) {
		auto ret = new ImageAreaSelectLayer();
		ret->m_data = std::move(data);
		if (ret->init(tex)) {
			ret->autorelease();
		} else {
			CC_SAFE_DELETE(ret);
		}
		return ret;
	}
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat) {
		if (down && key == enumKeyCodes::KEY_F2) {
			auto director = CCDirector::sharedDirector();
			auto winSize = director->getWinSize();
			auto scene = director->getRunningScene();


			auto captureSize = CCSize(1920, 1080);
			RenderTexture texture(captureSize.width, captureSize.height);
			auto data = texture.captureData(scene);

			if (director->getKeyboardDispatcher()->getShiftKeyPressed()) {
				ImageAreaSelectLayer::create(texture.intoTexture(), std::move(data))->show();
			} else {
				copy_screenshot(std::move(data), captureSize);
			}
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
	}
};
