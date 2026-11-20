#include "waveform.hpp"
#include <memory>

static const std::string vertShader = R"(
	attribute vec4 a_position;
	attribute vec2 a_texCoord;
	attribute vec4 a_color;

	#ifdef GL_ES
	varying mediump vec2 v_texCoord;
	#else
	varying vec2 v_texCoord;
	#endif

	void main()	{
		gl_Position = CC_MVPMatrix * a_position;
		v_texCoord = a_texCoord;
	}
)";

#define RELOAD_SHADERS 0

char fragShader[] = {
	#embed "shader.glsl"
};

std::string getFragShader() {
#if RELOAD_SHADERS
	return geode::utils::file::readString(asp::fs::Path(__FILE__).parent().value() / "shader.glsl").unwrap();
#else
	return std::string(fragShader, sizeof(fragShader));
#endif
}

CCGLProgram* makeShader() {
	CCGLProgram* prg = new CCGLProgram();
	if (!prg->initWithVertexShaderByteArray(vertShader.c_str(), getFragShader().c_str())) {
		delete prg;
		return nullptr;
	}
	prg->autorelease();
	prg->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
	prg->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
	prg->link();
	prg->updateUniforms();
	return prg;
}

CCGLProgram* getShader() {
#if RELOAD_SHADERS
	return makeShader();
#else
	auto* cache = CCShaderCache::sharedShaderCache();
	auto* shader = cache->programForKey("waveform"_spr);
	if (shader == nullptr) {
		shader = makeShader();
		if (!shader) return nullptr;
		cache->addProgram(shader, "waveform"_spr);
	}
	return shader;
#endif
}

bool WaveformNode::init(CCSize size) {
	CCNode::init();

	this->setAnchorPoint(ccp(0.5, 0.5));
	this->setContentSize(size);

	auto winSize = CCDirector::get()->getWinSize();
	auto frameSize = CCDirector::get()->getOpenGLView()->getFrameSize();
	float pixel = frameSize.width / winSize.width;
	auto factor = CCDirector::get()->getContentScaleFactor();

	auto textureSize = CCSize(int(size.width * pixel), 1.0f);
	auto data = std::make_unique<uint8_t[]>(textureSize.width * textureSize.height);

	for (int i = 0; i < textureSize.width; i++) {
		data[i] = (std::sin(float(i) * 0.05f) * 0.3f + 0.7f) * 255.f;
	}

	auto* texture = new CCTexture2D();
	texture->autorelease();
	texture->initWithData(data.get(), kCCTexture2DPixelFormat_I8, textureSize.width, textureSize.height, textureSize);
	texture->setAliasTexParameters();
	auto* shader = getShader();
	if (shader) {
		texture->setShaderProgram(shader);
	}
	m_texture = texture;

	return true;
}

void WaveformNode::setPercent(float percent) {
	auto* shader = m_texture->getShaderProgram();
	if (!shader) return;
	shader->use();
	shader->setUniformLocationWith1f(shader->getUniformLocationForName("percentage"), percent);
}

void WaveformNode::drawSamples(std::span<const float> samples) {
	auto textureSize = m_texture->getContentSizeInPixels();
	auto data = std::make_unique<uint8_t[]>(textureSize.width);

	for (int x = 0; x < textureSize.width; x++) {
		int index = static_cast<int>(x * samples.size() / textureSize.width);
		index = std::clamp(index, 0, int(samples.size()) - 1);
		float height = samples[index];
		data[x] = int(std::clamp(height, 0.f, 1.f) * 255.f);
	}

	// this is fine, the old gl texture is deleted properly
	m_texture->initWithData(data.get(), kCCTexture2DPixelFormat_I8, textureSize.width, textureSize.height, textureSize);
	m_texture->setAliasTexParameters();
	auto* shader = getShader();
	if (shader) {
		m_texture->setShaderProgram(shader);
	}
}

void WaveformNode::draw() {
	if (!this->isVisible()) return;

	m_texture->drawInRect(CCRect({}, this->getContentSize()));
}