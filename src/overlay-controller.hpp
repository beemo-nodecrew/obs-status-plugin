#pragma once

#include "overlay-settings.hpp"
#include "recording-state.hpp"
#include "window-utils.hpp"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>

class MicMonitor;
class OverlayWindow;
class SettingsDialog;
class QWidget;
class QScreen;

class OverlayController : public QObject {
	Q_OBJECT

public:
	explicit OverlayController(QObject *parent = nullptr);
	~OverlayController() override;

	void start();
	void openSettings(QWidget *parent);
	void refreshMic();
	void setRecordingState(RecordingState state);
	RecordingState recordingState() const { return state_; }

	const OverlaySettings &settings() const { return settings_; }
	void applySettings(const OverlaySettings &settings);

	const CaptureExclusionResult &captureExclusion() const { return exclusion_; }

private:
	QScreen *targetScreen() const;
	void placeWindow();
	void applyCaptureExclusion();
	void updateTaskbarRect(QScreen *screen);
	void updateMicState();
	qint64 elapsedMs() const;
	void initElapsedFromOutput();

	OverlaySettings settings_;
	RecordingState state_ = RecordingState::Stopped;
	QPointer<OverlayWindow> window_;
	QPointer<SettingsDialog> dialog_;
	MicMonitor *mic_ = nullptr;
	QTimer topmostTimer_;
	QTimer tickTimer_;
	QElapsedTimer runTimer_;
	qint64 accumulatedMs_ = 0;
	CaptureExclusionResult exclusion_;
};
