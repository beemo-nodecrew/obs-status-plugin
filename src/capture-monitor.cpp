#include "capture-monitor.hpp"

#include <plugin-support.h>

#include <QMetaObject>

CaptureMonitor::CaptureMonitor(QObject *parent) : QObject(parent)
{
	signal_handler_connect(obs_get_signal_handler(), "source_rename", onSourceRename, this);
}

CaptureMonitor::~CaptureMonitor()
{
	signal_handler_disconnect(obs_get_signal_handler(), "source_rename", onSourceRename, this);
}

void CaptureMonitor::setSourceName(const QString &name)
{
	if (name == name_)
		return;
	name_ = name;
	refresh();
}

void CaptureMonitor::refresh()
{
	SourceLabel next;
	uint32_t width = 0;
	uint32_t height = 0;

	if (!name_.isEmpty()) {
		next.visible = true;
		obs_source_t *source = obs_get_source_by_name(name_.toUtf8().constData());
		if (source && obs_source_removed(source)) {
			obs_source_release(source);
			source = nullptr;
		}

		if (source) {
			QString text;
			ListLabel result = list_property_label(source, "monitor_id", &text);
			if (result == ListLabel::NoProperty)
				result = list_property_label(source, "monitor", &text); // 구버전 정수 설정
			width = obs_source_get_width(source);
			height = obs_source_get_height(source);

			if (result == ListLabel::NoProperty) {
				next.found = true;
				next.text = elide_label(name_);
			} else if (result == ListLabel::Found && width > 0 && height > 0) {
				next.found = true;
				next.text = elide_label(trim_monitor_label(text));
			}
			obs_source_release(source);
		}
		if (!next.found)
			next.text = not_found_text();
	}

	if (next == label_)
		return;
	label_ = next;
	if (label_.visible)
		obs_log(LOG_INFO, "capture source '%s': %s (found: %s, %ux%u)", name_.toUtf8().constData(),
			label_.text.toUtf8().constData(), label_.found ? "yes" : "no", width, height);
	emit labelChanged();
}

void CaptureMonitor::onSourceRename(void *data, calldata_t *cd)
{
	auto *self = static_cast<CaptureMonitor *>(data);
	const QString newName = QString::fromUtf8(calldata_string(cd, "new_name"));
	const QString prevName = QString::fromUtf8(calldata_string(cd, "prev_name"));
	QMetaObject::invokeMethod(
		self,
		[self, newName, prevName]() {
			if (self->name_.isEmpty() || prevName != self->name_)
				return;
			self->name_ = newName;
			obs_log(LOG_INFO, "capture source renamed to '%s'", newName.toUtf8().constData());
			emit self->sourceRenamed(newName);
			self->refresh();
		},
		Qt::QueuedConnection);
}
