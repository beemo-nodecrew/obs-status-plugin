#pragma once

#include <obs.h>

#include <QString>

// 캡슐에 표시할 소스 이름 (녹화 화면, 마이크 장치)
struct SourceLabel {
	bool visible = false; // false면 캡슐에서 구간 숨김
	bool found = false;   // false면 빨간색
	QString text;         // 표시 문구 (found=false면 Overlay.NotFound 문구)

	bool operator==(const SourceLabel &o) const
	{
		return visible == o.visible && found == o.found && text == o.text;
	}
	bool operator!=(const SourceLabel &o) const { return !(*this == o); }
};

enum class ListLabel {
	Found,      // 현재 설정값과 같은 활성 항목이 목록에 있음
	Missing,    // 목록 속성은 있지만 현재 값이 없거나 비활성
	NoProperty, // 해당 이름의 목록 속성이 없음
};

// 소스 속성 prop(목록)에서 소스 설정의 현재 값과 같은 활성 항목을 찾아 *label에 문구 저장
ListLabel list_property_label(obs_source_t *source, const char *prop, QString *label);

// "DELL U2720Q: 2560x1440 @ 1920,0" → "DELL U2720Q"
QString trim_monitor_label(const QString &label);

// maxChars를 넘으면 maxChars-1자 + '…'
QString elide_label(const QString &label, int maxChars = 24);

QString not_found_text();
