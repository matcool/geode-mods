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

	// inside of WM_SIZING (0x214) case
	// nops a call to some internal function
	auto patch1 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0xd6b1a), {0x90, 0x90, 0x90, 0x90, 0x90}));
	
	// these both are required for fixing maximize

	// nops a call to SetWindowPos
	(void) Mod::get()->patch((void*)(base::getCocos() + 0xd4cd9), {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});

	// my patches :3
	// inside of WM_SIZE (5) case, ignore when wParam is SIZE_MAXIMIZED (2)
	// jnz -> jmp
	(void) Mod::get()->patch((void*)(base::getCocos() + 0xd61b7), {0x48, 0xe9});

	const auto togglePatches = [=](bool value) {
		// truly the code of all time
		if (value) {
			if (patch1) (void) patch1->enable();
		} else {
			if (patch1) (void) patch1->disable();
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