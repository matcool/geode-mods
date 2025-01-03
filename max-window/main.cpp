#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Setting.hpp>

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

	static_assert(GEODE_COMP_GD_VERSION == 22074, "max-window addresses are outdated!");

	// most of these can be found inside of glfw's WindowProc handler

	// inside of WM_SIZING (0x214) case
	// nops a call to some internal function
	auto patch1 = unwrapOrWarn(Mod::get()->patch((void*)(base::getCocos() + 0xd6eca), {0x90, 0x90, 0x90, 0x90, 0x90}));

	// these both are required for fixing maximize

	// nops a call to SetWindowPos
	(void) Mod::get()->patch((void*)(base::getCocos() + 0xd5089), {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});

	// my patches :3
	// inside of WM_SIZE (5) case, ignore when wParam is SIZE_MAXIMIZED (2)
	// jnz -> jmp
	(void) Mod::get()->patch((void*)(base::getCocos() + 0xd6567), {0x48, 0xe9});

	const auto togglePatches = [=](bool value) {
		// truly the code of all time
		if (value) {
			if (patch1) (void) patch1->enable();
		} else {
			if (patch1) (void) patch1->disable();
		}
	};

	togglePatches(Mod::get()->getSettingValue<bool>("free-resize"));

	listenForSettingChanges<bool>("free-resize", [=](bool value) {
		togglePatches(value);
	});

	if (Mod::get()->getSettingValue<bool>("start-maximized")) {
		ShowWindow(GetActiveWindow(), SW_MAXIMIZE);
	}
}