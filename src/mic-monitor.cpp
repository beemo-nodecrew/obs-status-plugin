#include "mic-monitor.hpp"
#include "mic-level-probe.hpp"
#include "audio-device-name.hpp"

#include <plugin-support.h>

#include <QMetaObject>

#include <cstring>

namespace {

// device_id 장치 목록 속성을 가진 오디오 소스 종류
bool has_device_list(const char *id)
{
	static const char *const kTypes[] = {"wasapi_input_capture",    "wasapi_output_capture",
					     "coreaudio_input_capture", "coreaudio_output_capture",
					     "pulse_input_capture",     "pulse_output_capture",
					     "alsa_input_capture"};
	if (!id)
		return false;
	for (const char *type : kTypes) {
		if (strcmp(id, type) == 0)
			return true;
	}
	return false;
}

} // namespace

MicMonitor::MicMonitor(QObject *parent) : QObject(parent), probe_(new MicLevelProbe(this))
{
	signal_handler_t *sh = obs_get_signal_handler();
	signal_handler_connect(sh, "source_create", onSourceCreate, this);
	signal_handler_connect(sh, "source_rename", onSourceRename, this);
	signal_handler_connect(sh, "source_remove", onSourceGone, this);
	signal_handler_connect(sh, "source_destroy", onSourceGone, this);
}

MicMonitor::~MicMonitor()
{
	signal_handler_t *sh = obs_get_signal_handler();
	signal_handler_disconnect(sh, "source_create", onSourceCreate, this);
	signal_handler_disconnect(sh, "source_rename", onSourceRename, this);
	signal_handler_disconnect(sh, "source_remove", onSourceGone, this);
	signal_handler_disconnect(sh, "source_destroy", onSourceGone, this);
	detach();
}

void MicMonitor::setSourceName(const QString &name)
{
	if (name == name_ && (weak_ || name.isEmpty()))
		return;
	name_ = name;
	refresh();
}

void MicMonitor::refresh()
{
	detach();
	attach();
}

SourceLabel MicMonitor::deviceLabel() const
{
	SourceLabel label;
	if (name_.isEmpty())
		return label;
	label.visible = true;

	obs_source_t *source = weak_ ? obs_weak_source_get_source(weak_) : nullptr;
	if (source) {
		const char *id = obs_source_get_unversioned_id(source);
		QString text;
		// 장치 목록이 없는 종류(영상 캡처 장치, 미디어 등)는 속성 조회 없이 소스 이름 표시
		const ListLabel result = has_device_list(id) ? list_property_label(source, "device_id", &text)
							     : ListLabel::NoProperty;
		switch (result) {
		case ListLabel::Found: {
			label.found = true;
			obs_data_t *settings = obs_source_get_settings(source);
			const bool isDefault = strcmp(obs_data_get_string(settings, "device_id"), "default") == 0;
			obs_data_release(settings);
			if (isDefault) {
				// "기본값" 대신 실제로 잡히는 장치 이름
				const QString actual =
					default_audio_device_name(strcmp(id, "wasapi_output_capture") != 0);
				if (!actual.isEmpty())
					text = actual;
			}
			label.text = elide_label(text);
			break;
		}
		case ListLabel::NoProperty:
			label.found = true;
			label.text = elide_label(name_);
			break;
		case ListLabel::Missing:
			break;
		}
		obs_source_release(source);
	}
	if (!label.found)
		label.text = not_found_text();
	return label;
}

void MicMonitor::setLevelLogEnabled(bool enabled)
{
	probe_->setEnabled(enabled);
}

void MicMonitor::attach()
{
	if (name_.isEmpty() || weak_) {
		if (name_.isEmpty())
			setMuted(false);
		return;
	}

	obs_source_t *source = obs_get_source_by_name(name_.toUtf8().constData());
	if (source && obs_source_removed(source)) {
		// 삭제됐지만 참조가 남아 이름 테이블에 남아 있는 소스는 무시
		obs_source_release(source);
		source = nullptr;
	}
	if (!source) {
		obs_log(LOG_INFO, "mic source '%s' not found", name_.toUtf8().constData());
		setMuted(false);
		return;
	}

	weak_ = obs_source_get_weak_source(source);
	attached_ = source;
	signal_handler_connect(obs_source_get_signal_handler(source), "mute", onMute, this);
	const bool muted = obs_source_muted(source);
	probe_->setSource(source);
	obs_source_release(source);

	obs_log(LOG_INFO, "mic source '%s' attached (muted: %s)", name_.toUtf8().constData(), muted ? "yes" : "no");
	setMuted(muted);
	emit attachedChanged(true);
}

void MicMonitor::detach()
{
	if (!weak_)
		return;
	probe_->setSource(nullptr);
	if (obs_source_t *source = obs_weak_source_get_source(weak_)) {
		signal_handler_disconnect(obs_source_get_signal_handler(source), "mute", onMute, this);
		obs_source_release(source);
	}
	obs_weak_source_release(weak_);
	weak_ = nullptr;
	attached_ = nullptr;
	emit attachedChanged(false);
}

void MicMonitor::setMuted(bool muted)
{
	if (muted_ == muted)
		return;
	muted_ = muted;
	emit mutedChanged(muted);
}

void MicMonitor::onMute(void *data, calldata_t *cd)
{
	auto *self = static_cast<MicMonitor *>(data);
	const bool muted = calldata_bool(cd, "muted");
	QMetaObject::invokeMethod(self, [self, muted]() { self->setMuted(muted); }, Qt::QueuedConnection);
}

void MicMonitor::onSourceCreate(void *data, calldata_t *cd)
{
	auto *self = static_cast<MicMonitor *>(data);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
	if (!source || !(obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
		return;
	const QString name = QString::fromUtf8(obs_source_get_name(source));
	QMetaObject::invokeMethod(
		self,
		[self, name]() {
			if (!self->weak_ && !self->name_.isEmpty() && name == self->name_)
				self->attach();
		},
		Qt::QueuedConnection);
}

void MicMonitor::onSourceRename(void *data, calldata_t *cd)
{
	auto *self = static_cast<MicMonitor *>(data);
	const QString newName = QString::fromUtf8(calldata_string(cd, "new_name"));
	const QString prevName = QString::fromUtf8(calldata_string(cd, "prev_name"));
	QMetaObject::invokeMethod(
		self,
		[self, newName, prevName]() {
			if (self->name_.isEmpty())
				return;
			if (prevName == self->name_ && self->weak_) {
				self->name_ = newName;
				obs_log(LOG_INFO, "mic source renamed to '%s'", newName.toUtf8().constData());
				emit self->sourceRenamed(newName);
			} else if (!self->weak_ && newName == self->name_) {
				self->attach();
			}
		},
		Qt::QueuedConnection);
}

void MicMonitor::onSourceGone(void *data, calldata_t *cd)
{
	auto *self = static_cast<MicMonitor *>(data);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
	QMetaObject::invokeMethod(
		self,
		[self, source]() {
			if (!source || source != self->attached_ || !self->weak_)
				return;
			// 주소 재사용 대비: 포인터 비교만으로 판단하지 않고 실제 소멸·삭제 여부 확인
			obs_source_t *current = obs_weak_source_get_source(self->weak_);
			const bool gone = !current || obs_source_removed(current);
			obs_source_release(current);
			if (!gone)
				return;
			obs_log(LOG_INFO, "mic source '%s' removed", self->name_.toUtf8().constData());
			self->detach();
			self->setMuted(false);
		},
		Qt::QueuedConnection);
}
