#include "mic-monitor.hpp"

#include <plugin-support.h>

#include <QMetaObject>

MicMonitor::MicMonitor(QObject *parent) : QObject(parent)
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

void MicMonitor::attach()
{
	if (name_.isEmpty() || weak_) {
		if (name_.isEmpty())
			setMuted(false);
		return;
	}

	obs_source_t *source = obs_get_source_by_name(name_.toUtf8().constData());
	if (!source) {
		obs_log(LOG_INFO, "mic source '%s' not found", name_.toUtf8().constData());
		setMuted(false);
		return;
	}

	weak_ = obs_source_get_weak_source(source);
	attached_ = source;
	signal_handler_connect(obs_source_get_signal_handler(source), "mute", onMute, this);
	const bool muted = obs_source_muted(source);
	obs_source_release(source);

	obs_log(LOG_INFO, "mic source '%s' attached (muted: %s)", name_.toUtf8().constData(), muted ? "yes" : "no");
	setMuted(muted);
}

void MicMonitor::detach()
{
	if (!weak_)
		return;
	if (obs_source_t *source = obs_weak_source_get_source(weak_)) {
		signal_handler_disconnect(obs_source_get_signal_handler(source), "mute", onMute, this);
		obs_source_release(source);
	}
	obs_weak_source_release(weak_);
	weak_ = nullptr;
	attached_ = nullptr;
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
			if (!source || source != self->attached_)
				return;
			obs_log(LOG_INFO, "mic source '%s' removed", self->name_.toUtf8().constData());
			self->detach();
			self->setMuted(false);
		},
		Qt::QueuedConnection);
}
