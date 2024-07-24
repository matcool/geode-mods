#include "RenderTexture.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// #include <CCGL.h>
#include <thread>

class MyRenderTexture : public CCRenderTexture {
public:
    unsigned char* getData() {
        const auto size = m_pTexture->getContentSizeInPixels();
        auto savedBufferWidth = (int)size.width;
        auto savedBufferHeight = (int)size.height;
        GLubyte* pTempData = nullptr;
        pTempData = new GLubyte[savedBufferWidth * savedBufferWidth * 4];
        this->begin();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, savedBufferWidth, savedBufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, pTempData);
        this->end();
        return pTempData;
    }
    const auto& getSizeInPixels() {
        return m_pTexture->getContentSizeInPixels();
    }
};

using uint = unsigned int;

void copy_screenshot(std::unique_ptr<uint8_t[]> data, const CCSize& size, uint x = 0, uint y = 0, uint a = 0, uint b = 0) {
    const auto src_width = static_cast<uint>(size.width);
    const auto src_height = static_cast<uint>(size.height);
    a = a ? a : src_width;
    b = b ? b : src_height;
    std::thread([=, data = std::move(data)]() {
        // auto area = static_cast<size_t>((a - x) * (b - y));
		// auto bytesPerPixel = 3;
        // auto buffer = reinterpret_cast<uint32_t*>(malloc(area * sizeof(uint32_t)));
        // auto cx = x, cy = y;
        // for (size_t i = 0; i < area * 4; ++i) {
        //     const size_t j = ((src_height - cy) * src_width + cx) * bytesPerPixel;
        //     if (++cx == a) ++cy, cx = x;
        //     buffer[i] = data[j] << 16 | data[j + 1] << 8 | data[j + 2];

        //     // const auto n = reinterpret_cast<uint32_t*>(data)[i];
        //     // buffer[i] = (n & 0x0000FF) << 16 | (n & 0x00FF00) | (n & 0xFF0000) >> 16;
        // }
        auto bitmap = CreateBitmap((int)size.width, (int)size.height, 1, 32, data.get());

        if (OpenClipboard(NULL))
            if (EmptyClipboard()) {
                SetClipboardData(CF_BITMAP, bitmap);
                CloseClipboard();
            }
        // free(buffer);
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
        sprite->setScale(0.75f);
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
            uint x = static_cast<uint>(std::clamp(xf * width, 0.f, width));
            uint y = static_cast<uint>(std::clamp((1.f - yf) * height, 0.f, height));
            uint a = static_cast<uint>(std::clamp(af * width, 0.f, width));
            uint b = static_cast<uint>(std::clamp((1.f - bf) * height, 0.f, height));
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
			auto data = texture.capture(scene);

			auto* ctexture = texture.intoTexture();

			if (director->getKeyboardDispatcher()->getShiftKeyPressed()) {
				ImageAreaSelectLayer::create(ctexture, std::move(data))->show();
			} else {
				copy_screenshot(std::move(data), captureSize);
			}
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
	}
};
