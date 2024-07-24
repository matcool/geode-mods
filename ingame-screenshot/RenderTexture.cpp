#include "RenderTexture.hpp"

using namespace geode::prelude;

RenderTexture::RenderTexture(unsigned int width, unsigned int height) 
	: m_width(width),
	m_height(height)
{
	// generate texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGB,
		static_cast<GLsizei>(m_width),
		static_cast<GLsizei>(m_height),
		0, GL_RGB, GL_UNSIGNED_BYTE, 0
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenRenderbuffers(1, &m_depthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencil);
	glRenderbufferStorage(
		GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
		static_cast<GLsizei>(m_width),
		static_cast<GLsizei>(m_height)
	);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencil);

	// attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
}

RenderTexture::~RenderTexture() {
	if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
	if (m_texture) glDeleteTextures(1, &m_texture);
	if (m_depthStencil) glDeleteRenderbuffers(1, &m_depthStencil);
}

void RenderTexture::begin() {
	// save old buffers
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_oldFBO);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &m_oldRBO);

	auto* glview = CCEGLView::get();
	auto* director = CCDirector::get();

	m_oldScaleX = glview->m_fScaleX;
	m_oldScaleY = glview->m_fScaleY;
	// this is used for scissoring, so otherwise clipping nodes would be wrong
	glview->m_fScaleX = m_width / director->getWinSize().width;
	glview->m_fScaleY = m_height / director->getWinSize().height;
	
	glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	// idk either tbh i just copied it from drawScene
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderTexture::end() {
	auto* glview = CCEGLView::get();
	auto* director = CCDirector::get();

	// restore saved buffers
	glBindRenderbuffer(GL_RENDERBUFFER, m_oldRBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_oldFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glview->m_fScaleX = m_oldScaleX;
	glview->m_fScaleY = m_oldScaleY;
	director->setViewport();
}

std::unique_ptr<uint8_t[]> RenderTexture::capture(CCNode* node, PixelFormat format) {
	this->begin();
	node->visit();
	auto data = this->readDataFromTexture(format);
	this->end();
	return data;
}

std::unique_ptr<uint8_t[]> RenderTexture::readDataFromTexture(PixelFormat format) {
	int perPixel = 4;
	int glFormat = GL_RGBA;
	switch (format) {
		case PixelFormat::BGR:
			perPixel = 3;
			glFormat = GL_BGR;
			break;
		case PixelFormat::RGB:
			perPixel = 3;
			glFormat = GL_RGB;
			break;
		case PixelFormat::BGRA:
			perPixel = 4;
			glFormat = GL_BGRA;
			break;
		case PixelFormat::RGBA:
			perPixel = 4;
			glFormat = GL_RGBA;
			break;
	}

	auto pixels = std::make_unique<uint8_t[]>(m_width * m_height * perPixel);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glReadPixels(0, 0, m_width, m_height, glFormat, GL_UNSIGNED_BYTE, pixels.get());
    
	return pixels;
}

struct HackCCTexture2D : CCTexture2D {
	bool initWithGLName(GLuint name, GLsizei pixelsWidth, GLsizei pixelsHeight, const CCSize& contentSize) {
		m_uName = name;

		m_tContentSize = contentSize;
		m_uPixelsWide = pixelsWidth;
		m_uPixelsHigh = pixelsHeight;
		m_ePixelFormat = kCCTexture2DPixelFormat_RGBA8888;
		m_fMaxS = contentSize.width / (float)(pixelsWidth);
		m_fMaxT = contentSize.height / (float)(pixelsHeight);

		m_bHasPremultipliedAlpha = false;
		m_bHasMipmaps = false;

		this->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTexture));
		return true;
	}
};

CCTexture2D* RenderTexture::intoTexture() {
	auto texture = new CCTexture2D();
	static_cast<HackCCTexture2D*>(texture)->initWithGLName(
		m_texture, m_width, m_height, CCSize(m_width, m_height)
	);
	texture->autorelease();
	m_texture = 0;
	return texture;
}