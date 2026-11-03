#pragma once

#include "overlay-settings.hpp"
#include "recording-state.hpp"
#include "window-utils.hpp"

#include <QObject>
#include <QPointer>
#include <QTimer>

class OverlayWindow;
class QScreen;

class OverlayController : public QObject {
	Q_OBJECT

public:
	explicit OverlayController(QObject *parent = nullptr);
	~OverlayController() override;

	void start();
	void setRecordingState(RecordingState state);
	RecordingState recordingState() const { return state_; }

	const OverlaySettings &settings() const { return settings_; }
	void applySettings(const OverlaySettings &settings);

	const CaptureExclusionResult &captureExclusion() const { return exclusion_; }

private:
	QScreen *targetScreen() const;
	void placeWindow();
	void applyCaptureExclusion();

	OverlaySettings settings_;
	RecordingState state_ = RecordingState::Stopped;
	QPointer<OverlayWindow> window_;
	QTimer topmostTimer_;
	CaptureExclusionResult exclusion_;
};
