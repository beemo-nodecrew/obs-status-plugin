#include "window-utils.hpp"

#include <QWidget>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

CaptureExclusionResult exclude_from_capture(QWidget *widget)
{
	CaptureExclusionResult result;
	result.supported = true;

	HWND hwnd = reinterpret_cast<HWND>(widget->winId());
	if (!hwnd) {
		result.message = QStringLiteral("no window handle");
		return result;
	}

	if (SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
		DWORD affinity = 0;
		if (GetWindowDisplayAffinity(hwnd, &affinity) && affinity == WDA_EXCLUDEFROMCAPTURE) {
			result.success = true;
			result.message = QStringLiteral("WDA_EXCLUDEFROMCAPTURE");
			return result;
		}
		result.message = QStringLiteral("affinity not applied (value %1)").arg(affinity);
		return result;
	}

	result.errorCode = GetLastError();
	result.message = QStringLiteral("SetWindowDisplayAffinity failed (error %1)").arg(result.errorCode);
	return result;
}

void raise_topmost(QWidget *widget)
{
	HWND hwnd = reinterpret_cast<HWND>(widget->winId());
	if (!hwnd)
		return;
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
		     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
}

#else

CaptureExclusionResult exclude_from_capture(QWidget *)
{
	CaptureExclusionResult result;
	result.message = QStringLiteral("not supported on this platform");
	return result;
}

void raise_topmost(QWidget *widget)
{
	widget->raise();
}

#endif
