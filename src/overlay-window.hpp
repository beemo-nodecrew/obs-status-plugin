#pragma once

#include "overlay-settings.hpp"
#include "recording-state.hpp"

#include <QWidget>

class QPainter;
class QScreen;

class OverlayWindow : public QWidget {
	Q_OBJECT

public:
	explicit OverlayWindow(QWidget *parent = nullptr);

	void setRecordingState(RecordingState state);
	void setMicState(bool available, bool muted);
	void setElapsed(qint64 ms);
	void applySettings(const OverlaySettings &settings);
	void moveToScreen(QScreen *screen);

	// 위젯 좌표 기준 작업 표시줄 영역 (없으면 빈 QRect)
	void setTaskbarRect(const QRect &rect);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	struct PillLayout {
		QRect rect;
		bool showStatus = false;
		bool showTime = false;
		bool showMic = false;
		int fontPx = 14;
	};

	PillLayout computeLayout() const;
	QRect pillRect(const QSize &size) const;
	void drawGlow(QPainter &painter, const QColor &color) const;
	void drawPill(QPainter &painter, const PillLayout &layout) const;
	QString statusText() const;
	QString elapsedText() const;

	OverlaySettings settings_;
	RecordingState state_ = RecordingState::Stopped;
	bool micAvailable_ = false;
	bool micMuted_ = false;
	qint64 elapsedMs_ = 0;
	QRect taskbarRect_;
	QRect lastPill_;
};
