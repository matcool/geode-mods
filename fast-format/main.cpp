#include <Geode/Geode.hpp>

using namespace geode::prelude;

bool myhook(CCString* self, const char* format, va_list ap) {
	int size = std::vsnprintf(nullptr, 0, format, ap);
	std::string str(size + 1, '\0');
	std::vsnprintf(str.data(), str.size(), format, ap);
	self->m_sString = str;
	return true;
}

$execute {
	auto addr = GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?initWithFormatAndValist@CCString@cocos2d@@AAE_NPBDPAD@Z");
	(void) Mod::get()->addHook(
		reinterpret_cast<void*>(addr),
		&myhook,
		"CCString::initWithFormatAndValist",
		tulip::hook::TulipConvention::Thiscall
	);
}