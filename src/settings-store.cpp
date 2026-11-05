#include "settings-store.hpp"

#include <obs-module.h>
#include <plugin-support.h>
#include <util/platform.h>

namespace {

const char *kFileName = "settings.json";
const int kSettingsVersion = 2;

QString get_qstring(obs_data_t *data, const char *key)
{
	return QString::fromUtf8(obs_data_get_string(data, key));
}

void set_defaults(obs_data_t *data, const OverlaySettings &d)
{
	obs_data_set_default_bool(data, "enabled", d.enabled);
	obs_data_set_default_string(data, "screen_name", d.screenName.toUtf8().constData());
	obs_data_set_default_int(data, "screen_index", d.screenIndex);
	obs_data_set_default_int(data, "position", static_cast<int>(d.position));
	obs_data_set_default_int(data, "font_size", d.fontSize);
	obs_data_set_default_int(data, "border_width", d.borderWidth);
	obs_data_set_default_bool(data, "show_rec", d.showRec);
	obs_data_set_default_bool(data, "show_stopped", d.showStopped);
	obs_data_set_default_bool(data, "show_elapsed", d.showElapsed);
	obs_data_set_default_string(data, "text_paused", d.textPaused.toUtf8().constData());
	obs_data_set_default_string(data, "text_stopped", d.textStopped.toUtf8().constData());
	obs_data_set_default_string(data, "text_rec", d.textRec.toUtf8().constData());
	obs_data_set_default_string(data, "mic_source", d.micSource.toUtf8().constData());
}

BadgePosition to_position(long long value)
{
	if (value < static_cast<int>(BadgePosition::TopLeft) || value > static_cast<int>(BadgePosition::BottomCenter))
		return BadgePosition::TaskbarCenter;
	return static_cast<BadgePosition>(value);
}

} // namespace

OverlaySettings load_overlay_settings()
{
	const OverlaySettings defaults;
	OverlaySettings s = defaults;

	char *path = obs_module_config_path(kFileName);
	if (!path)
		return s;

	obs_data_t *data = obs_data_create_from_json_file_safe(path, "bak");
	bfree(path);
	if (!data)
		data = obs_data_create();

	set_defaults(data, defaults);
	s.enabled = obs_data_get_bool(data, "enabled");
	s.screenName = get_qstring(data, "screen_name");
	s.screenIndex = static_cast<int>(obs_data_get_int(data, "screen_index"));
	s.position = to_position(obs_data_get_int(data, "position"));
	s.fontSize = qBound(10, static_cast<int>(obs_data_get_int(data, "font_size")), 200);
	s.borderWidth = qBound(0, static_cast<int>(obs_data_get_int(data, "border_width")), 100);
	s.showRec = obs_data_get_bool(data, "show_rec");
	s.showStopped = obs_data_get_bool(data, "show_stopped");
	s.showElapsed = obs_data_get_bool(data, "show_elapsed");
	s.textPaused = get_qstring(data, "text_paused");
	s.textStopped = get_qstring(data, "text_stopped");
	s.textRec = get_qstring(data, "text_rec");
	s.micSource = get_qstring(data, "mic_source");

	// v1 → v2: 배지 디자인 변경에 맞춰 위치·크기·문구 기본값 이전 (사용자가 바꾼 값은 유지)
	const bool migrate = obs_data_get_int(data, "settings_version") < 2;
	if (migrate) {
		s.position = defaults.position;
		if (s.fontSize == 28)
			s.fontSize = defaults.fontSize;
		if (s.borderWidth == 8)
			s.borderWidth = defaults.borderWidth;
		if (s.textRec == QStringLiteral("REC"))
			s.textRec = defaults.textRec;
		obs_log(LOG_INFO, "settings migrated to version %d", kSettingsVersion);
	}
	obs_data_release(data);
	if (migrate)
		save_overlay_settings(s);
	return s;
}

bool save_overlay_settings(const OverlaySettings &s)
{
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	char *path = obs_module_config_path(kFileName);
	if (!path)
		return false;

	obs_data_t *data = obs_data_create();
	obs_data_set_int(data, "settings_version", kSettingsVersion);
	obs_data_set_bool(data, "enabled", s.enabled);
	obs_data_set_string(data, "screen_name", s.screenName.toUtf8().constData());
	obs_data_set_int(data, "screen_index", s.screenIndex);
	obs_data_set_int(data, "position", static_cast<int>(s.position));
	obs_data_set_int(data, "font_size", s.fontSize);
	obs_data_set_int(data, "border_width", s.borderWidth);
	obs_data_set_bool(data, "show_rec", s.showRec);
	obs_data_set_bool(data, "show_stopped", s.showStopped);
	obs_data_set_bool(data, "show_elapsed", s.showElapsed);
	obs_data_set_string(data, "text_paused", s.textPaused.toUtf8().constData());
	obs_data_set_string(data, "text_stopped", s.textStopped.toUtf8().constData());
	obs_data_set_string(data, "text_rec", s.textRec.toUtf8().constData());
	obs_data_set_string(data, "mic_source", s.micSource.toUtf8().constData());

	const bool ok = obs_data_save_json_safe(data, path, "tmp", "bak");
	obs_data_release(data);
	if (!ok)
		obs_log(LOG_WARNING, "failed to save settings: %s", path);
	bfree(path);
	return ok;
}
