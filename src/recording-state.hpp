#pragma once

#include <obs-frontend-api.h>

enum class RecordingState {
	Stopped,
	Recording,
	Paused,
};

inline const char *recording_state_name(RecordingState state)
{
	switch (state) {
	case RecordingState::Recording:
		return "recording";
	case RecordingState::Paused:
		return "paused";
	case RecordingState::Stopped:
	default:
		return "stopped";
	}
}

inline RecordingState query_recording_state()
{
	if (!obs_frontend_recording_active())
		return RecordingState::Stopped;
	return obs_frontend_recording_paused() ? RecordingState::Paused : RecordingState::Recording;
}
