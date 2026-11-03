#pragma once

#include "overlay-settings.hpp"
#include "recording-state.hpp"

#include <QObject>
#include <QPointer>

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

private:
	QScreen *targetScreen() const;
	void placeWindow();

	OverlaySettings settings_;
	RecordingState state_ = RecordingState::Stopped;
	QPointer<OverlayWindow> window_;
};
