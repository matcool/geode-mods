// MIT License
// 
// Copyright (c) 2024 mat
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#pragma once

#include <Geode/Geode.hpp>
#include <memory>

class RenderTexture {
	unsigned int m_width = 0;
	unsigned int m_height = 0;
	GLint m_oldFBO = 0;
	GLint m_oldRBO = 0;
	GLuint m_fbo = 0;
	GLuint m_depthStencil = 0;
	GLuint m_texture = 0;
	float m_oldScaleX = 0;
	float m_oldScaleY = 0;
	bool m_fbActive = false;

	cocos2d::CCTexture2D* asTexture();
public:
	RenderTexture(unsigned int width, unsigned int height);
	RenderTexture(RenderTexture&&);
	~RenderTexture();

	enum class PixelFormat {
		RGB,
		BGR,
		RGBA,
		BGRA
	};

	// begin, visit, end a node
	void capture(cocos2d::CCNode* node, PixelFormat format = PixelFormat::RGBA);
	std::unique_ptr<uint8_t[]> captureData(cocos2d::CCNode* node, PixelFormat format = PixelFormat::RGBA);

	void begin();
	void end();

	std::unique_ptr<uint8_t[]> readDataFromTexture(PixelFormat format = PixelFormat::RGBA);

	GLuint getTexture() const { return m_texture; }
	
	// creates a CCTexture2D from the internal texture. this class should not be used after this
	// as cocos now owns the texture, and a new one isnt created
	cocos2d::CCTexture2D* intoTexture();

	class Sprite;

	// creates a CCSprite from this texture, while keeping the texture alive. this class should not be used after this,
	// use return->render instead
	std::shared_ptr<Sprite> intoManagedSprite();
};

class RenderTexture::Sprite {
	Sprite(const Sprite&) = delete;
	Sprite& operator=(const Sprite&) = delete;
public:
	Sprite(RenderTexture texture);
	~Sprite();

	// these two must live together since they both share an opengl texture
	// and thus both will try to delete it

	geode::Ref<cocos2d::CCSprite> sprite;
	RenderTexture render;
};