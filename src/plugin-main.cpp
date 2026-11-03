/*
OBS Status Overlay
Copyright (C) 2026 beemo-nodecrew <beemo@nodecrew.kr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QAction>
#include <QMainWindow>

#include "overlay-controller.hpp"
#include "recording-state.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

RecordingState current_state = RecordingState::Stopped;
OverlayController *controller = nullptr;

void set_state(RecordingState state, const char *reason)
{
	if (state == current_state) {
		obs_log(LOG_DEBUG, "recording state unchanged: %s (%s)", recording_state_name(state), reason);
		return;
	}
	obs_log(LOG_INFO, "recording state: %s -> %s (%s)", recording_state_name(current_state),
		recording_state_name(state), reason);
	current_state = state;
	if (controller)
		controller->setRecordingState(state);
}

void on_frontend_event(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		current_state = query_recording_state();
		obs_log(LOG_INFO, "initial recording state: %s", recording_state_name(current_state));
		if (!controller) {
			controller = new OverlayController();
			controller->setRecordingState(current_state);
			controller->start();
		}
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		set_state(RecordingState::Recording, "started");
		break;
	case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		set_state(RecordingState::Paused, "paused");
		break;
	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
		set_state(RecordingState::Recording, "unpaused");
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		set_state(RecordingState::Stopped, "stopped");
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		if (controller)
			controller->refreshMic();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		obs_log(LOG_INFO, "frontend exit");
		delete controller;
		controller = nullptr;
		break;
	default:
		break;
	}
}

} // namespace

bool obs_module_load(void)
{
	obs_frontend_add_event_callback(on_frontend_event, nullptr);

	auto *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(obs_module_text("Menu.Title")));
	QObject::connect(action, &QAction::triggered, []() {
		if (controller)
			controller->openSettings(static_cast<QMainWindow *>(obs_frontend_get_main_window()));
	});
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(on_frontend_event, nullptr);
	obs_log(LOG_INFO, "plugin unloaded");
}
