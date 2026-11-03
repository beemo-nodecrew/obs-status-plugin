#pragma once

#include <QString>

class QWidget;

struct CaptureExclusionResult {
	bool supported = false;
	bool success = false;
	unsigned long errorCode = 0;
	QString message;
};

// 녹화·화면 캡처에서 창 제외 (Windows 10 2004 이상: WDA_EXCLUDEFROMCAPTURE)
CaptureExclusionResult exclude_from_capture(QWidget *widget);

// 다른 최상위 창보다 위로 재지정 (포커스 이동 없음)
void raise_topmost(QWidget *widget);
