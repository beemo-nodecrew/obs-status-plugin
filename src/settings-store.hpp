#pragma once

#include "overlay-settings.hpp"

OverlaySettings load_overlay_settings();
bool save_overlay_settings(const OverlaySettings &settings);
