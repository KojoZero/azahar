// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "core/hle/service/cfg/cfg.h"

namespace LibRetro {

enum CStickFunction { Both, CStick, Touchscreen, Toggle};

enum AnalogToggleState{ ToggledMain, ToggledAlternate};


struct CoreSettings {

    ::std::string file_path;

    float deadzone = 1.f;

    int maxspeed;

    float responsecurve;

    float speedupratio;

    bool speedup_enabled;

    LibRetro::CStickFunction analog_function;

    LibRetro::AnalogToggleState analog_toggle;

    bool swap_screen_state;

    bool inverted_swap_screen_state;

    bool initializedLayout;

    bool analog_cstick_enabled;

    bool analog_touch_enabled;

    bool mouse_touchscreen;

    Service::CFG::SystemLanguage language_value;

    bool touch_touchscreen;

    bool render_touchscreen;

    bool toggle_swap_screen;

} extern settings;

void RegisterCoreOptions(void);
void ParseCoreOptions(void);

} // namespace LibRetro
