#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingEvent.hpp>

using namespace geode::prelude;

$execute {
	const auto unwrapOrWarn = [](const auto& result) -> Patch* {
		if (result.isErr()) {
			log::warn("Unable to place a patch! {}", result.unwrapErr());
			return nullptr;
		} else {
			return result.unwrap();
		}
	};

	// most of these can be found inside of glfw's WindowProc handler

	// all copied from mhv5
	// inside of WM_SIZING case
	auto patch1 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0x118a2b), {0x90, 0x90, 0x90, 0x90, 0x90}));
	// patches a function call to:
	// xor ecx, ecx
	// jmp 0x6 (to some false case nearby)
	auto patch2 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0x11853d), {0x31, 0xc9, 0xeb, 0x06, 0x90}));
	// makes some instruction use ecx instead of edx
	auto patch3 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0x11855f + 1), {0x48}));
	auto patch4 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0x118565 + 1), {0x48}));
	
	// these both are required for fixing maximize

	// jumps over a call to SetWindowPos
	(void) Mod::get()->patch((void*)(base::getCocos() + 0x1176d6), {0xeb, 0x11, 0x90});

	// my patches :3
	// inside of WM_SIZE case, ignore when wParam is SIZE_MAXIMIZED (2)
	(void) Mod::get()->patch((void*)(base::getCocos() + 0x118421), {0xe9, 0x2f, 0xff, 0xff, 0xff, 0x90});

	const auto togglePatches = [=](bool value) {
		// truly the code of all time
		if (value) {
			if (patch1) (void) patch1->enable();
			if (patch2) (void) patch2->enable();
			if (patch3) (void) patch3->enable();
			if (patch4) (void) patch4->enable();
		} else {
			if (patch1) (void) patch1->disable();
			if (patch2) (void) patch2->disable();
			if (patch3) (void) patch3->disable();
			if (patch4) (void) patch4->disable();
		}
	};

	togglePatches(Mod::get()->getSettingValue<bool>("free-resize"));

	new EventListener([=](bool value) {
		togglePatches(value);
	}, GeodeSettingChangedFilter<bool>(getMod()->getID(), "free-resize"));

	if (Mod::get()->getSettingValue<bool>("start-maximized")) {
		ShowWindow(GetActiveWindow(), SW_MAXIMIZE);
	}
}