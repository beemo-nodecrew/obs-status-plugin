#pragma once

#include <QString>

enum class BadgePosition {
	TopLeft = 0,
	TopCenter = 1,
	TopRight = 2,
	BottomLeft = 3,
	BottomRight = 4,
};

struct OverlaySettings {
	bool enabled = true;
	QString screenName;
	int screenIndex = 0;
	BadgePosition position = BadgePosition::TopRight;
	int fontSize = 28;
	int borderWidth = 8;
	bool showRec = true;
	bool showStopped = true;
	QString textPaused = QStringLiteral("녹화 일시정지");
	QString textStopped = QStringLiteral("녹화 안 함");
	QString textRec = QStringLiteral("REC");
	QString micSource;
	bool micLevelLog = false; // 진단용, 저장하지 않음 (OBS 재시작 시 꺼짐)
};
