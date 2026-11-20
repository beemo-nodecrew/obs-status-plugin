#include "source-label.hpp"

#include <obs-module.h>

#include <cstring>

ListLabel list_property_label(obs_source_t *source, const char *prop, QString *label)
{
	if (!source || !prop)
		return ListLabel::NoProperty;

	// 소스 인스턴스가 아닌 소스 종류 기준으로 속성 조회:
	// obs_source_properties는 속성 변경 콜백을 실행해 디스플레이 캡처를 다시 초기화하므로 쓰지 않음
	obs_properties_t *props = obs_get_source_properties(obs_source_get_id(source));
	if (!props)
		return ListLabel::NoProperty;

	ListLabel result = ListLabel::NoProperty;
	obs_property_t *p = obs_properties_get(props, prop);
	if (p && obs_property_get_type(p) == OBS_PROPERTY_LIST) {
		result = ListLabel::Missing;
		obs_data_t *settings = obs_source_get_settings(source);
		const enum obs_combo_format format = obs_property_list_format(p);
		const size_t count = obs_property_list_item_count(p);

		for (size_t i = 0; i < count; ++i) {
			bool match = false;
			if (format == OBS_COMBO_FORMAT_STRING) {
				const char *current = obs_data_get_string(settings, prop);
				const char *item = obs_property_list_item_string(p, i);
				match = current && *current && item && strcmp(current, item) == 0;
			} else if (format == OBS_COMBO_FORMAT_INT) {
				match = obs_data_get_int(settings, prop) == obs_property_list_item_int(p, i);
			}
			if (!match)
				continue;
			if (!obs_property_list_item_disabled(p, i)) {
				result = ListLabel::Found;
				if (label)
					*label = QString::fromUtf8(obs_property_list_item_name(p, i));
			}
			break;
		}
		obs_data_release(settings);
	}
	obs_properties_destroy(props);
	return result;
}

QString trim_monitor_label(const QString &label)
{
	const qsizetype colon = label.indexOf(QLatin1Char(':'));
	const QString head = colon > 0 ? label.left(colon) : label;
	return head.trimmed();
}

QString elide_label(const QString &label, int maxChars)
{
	if (maxChars < 2 || label.size() <= maxChars)
		return label;
	return label.left(maxChars - 1) + QChar(0x2026);
}

QString not_found_text()
{
	return QString::fromUtf8(obs_module_text("Overlay.NotFound"));
}
