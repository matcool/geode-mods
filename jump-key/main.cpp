#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool a, bool b) {
		switch (key) {
			// turn C into W
			case KEY_C: key = KEY_W; break;
			// turn JIL into <^>
			case KEY_J: key = KEY_Left; break;
			case KEY_I: key = KEY_Up; break;
			case KEY_L: key = KEY_Right; break;
			// i find this comfortable ok
			case KEY_K: key = KEY_Left; break;
			default: break;
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, a, b);
	}
};