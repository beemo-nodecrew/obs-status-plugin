#pragma once

#include "overlay-settings.hpp"
#include "recording-state.hpp"

#include <QWidget>

class QScreen;

class OverlayWindow : public QWidget {
	Q_OBJECT

public:
	explicit OverlayWindow(QWidget *parent = nullptr);

	void setRecordingState(RecordingState state);
	void setMicMuted(bool muted);
	void applySettings(const OverlaySettings &settings);
	void moveToScreen(QScreen *screen);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	void drawBorder(class QPainter &painter, const QColor &color);
	QRect badgeRect(const QSize &size) const;

	OverlaySettings settings_;
	RecordingState state_ = RecordingState::Stopped;
	bool micMuted_ = false;
};
