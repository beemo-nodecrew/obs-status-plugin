#pragma once

#include <QString>

enum class BadgePosition {
	TopLeft = 0,
	TopCenter = 1,
	TopRight = 2,
	BottomLeft = 3,
	BottomRight = 4,
	TaskbarCenter = 5, // 가로 중앙, 작업 표시줄 높이 안
	BottomCenter = 6,
};

struct OverlaySettings {
	bool enabled = true;
	QString screenName;
	int screenIndex = 0;
	BadgePosition position = BadgePosition::TaskbarCenter;
	int fontSize = 14;
	int borderWidth = 6;
	bool showRec = true;
	bool showStopped = true;
	bool showElapsed = true;
	QString textPaused = QStringLiteral("녹화 일시정지");
	QString textStopped = QStringLiteral("녹화 안 함");
	QString textRec = QStringLiteral("녹화 중");
	QString micSource;
	bool micLevelLog = false; // 진단용, 저장하지 않음 (OBS 재시작 시 꺼짐)
};
