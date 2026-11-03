#include "mic-level-probe.hpp"

#include <plugin-support.h>

#include <QMetaObject>

#include <cmath>
#include <cstring>

namespace {

const char *kProbeSourceName = "obs-status-overlay-mic-probe";

// 같은 장치를 한 번 더 열어도 안전한 오디오 입력 캡처 소스만 원본 측정 대상으로 허용
bool is_audio_input_capture(const char *id)
{
	if (!id)
		return false;
	static const char *const kIds[] = {"wasapi_input_capture", "coreaudio_input_capture", "pulse_input_capture",
					   "alsa_input_capture"};
	for (const char *known : kIds) {
		if (std::strcmp(id, known) == 0)
			return true;
	}
	return false;
}

QString format_db(double linear)
{
	if (linear <= 0.0)
		return QStringLiteral("-inf");
	return QString::number(20.0 * std::log10(linear), 'f', 1);
}

} // namespace

MicLevelProbe::MicLevelProbe(QObject *parent) : QObject(parent)
{
	timer_.setInterval(1000);
	connect(&timer_, &QTimer::timeout, this, &MicLevelProbe::report);
}

MicLevelProbe::~MicLevelProbe()
{
	stop();
	obs_weak_source_release(weak_);
}

void MicLevelProbe::setEnabled(bool enabled)
{
	if (enabled_ == enabled)
		return;
	enabled_ = enabled;
	if (enabled_)
		start();
	else
		stop();
}

void MicLevelProbe::setSource(obs_source_t *source)
{
	stop();
	obs_weak_source_release(weak_);
	weak_ = source ? obs_source_get_weak_source(source) : nullptr;
	if (enabled_)
		start();
}

void MicLevelProbe::start()
{
	if (running_ || !weak_)
		return;
	obs_source_t *source = obs_weak_source_get_source(weak_);
	if (!source)
		return;

	const size_t channels = audio_output_get_channels(obs_get_audio());
	resetMeter(filteredMeter_, channels);
	resetMeter(rawMeter_, channels);

	obs_source_add_audio_capture_callback(source, onAudio, &filteredMeter_);
	signal_handler_connect(obs_source_get_signal_handler(source), "update", onUpdate, this);
	createRaw(source);
	running_ = true;
	timer_.start();

	obs_log(LOG_INFO, "mic level logging started for '%s' (raw: %s)", obs_source_get_name(source),
		raw_ ? "ok" : rawNote_.toUtf8().constData());
	obs_source_release(source);
}

void MicLevelProbe::stop()
{
	if (!running_)
		return;
	timer_.stop();
	destroyRaw();
	if (obs_source_t *source = obs_weak_source_get_source(weak_)) {
		obs_source_remove_audio_capture_callback(source, onAudio, &filteredMeter_);
		signal_handler_disconnect(obs_source_get_signal_handler(source), "update", onUpdate, this);
		obs_source_release(source);
	}
	running_ = false;
	obs_log(LOG_INFO, "mic level logging stopped");
}

void MicLevelProbe::createRaw(obs_source_t *source)
{
	rawNote_.clear();
	if (!is_audio_input_capture(obs_source_get_unversioned_id(source))) {
		rawNote_ = QStringLiteral("unsupported source type '%1'")
				   .arg(QString::fromUtf8(obs_source_get_unversioned_id(source)));
		return;
	}

	// 설정 객체를 공유하면 원본 소스 설정이 함께 바뀌므로 복사본으로 생성
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_t *copy = obs_data_create();
	obs_data_apply(copy, settings);
	obs_data_release(settings);
	raw_ = obs_source_create_private(obs_source_get_id(source), kProbeSourceName, copy);
	obs_data_release(copy);
	if (!raw_) {
		rawNote_ = QStringLiteral("failed to create probe source");
		return;
	}

	obs_source_add_audio_capture_callback(raw_, onAudio, &rawMeter_);
	// 입력 캡처 소스는 활성 상태일 때만 장치를 열기 때문에 직접 활성화
	obs_source_inc_active(raw_);
}

void MicLevelProbe::destroyRaw()
{
	if (!raw_)
		return;
	obs_source_remove_audio_capture_callback(raw_, onAudio, &rawMeter_);
	obs_source_dec_active(raw_);
	obs_source_release(raw_);
	raw_ = nullptr;
}

void MicLevelProbe::report()
{
	const Snapshot filtered = takeSnapshot(filteredMeter_);
	QString raw;
	if (raw_)
		raw = formatSnapshot(takeSnapshot(rawMeter_));
	else
		raw = QStringLiteral("n/a (%1)").arg(rawNote_);

	obs_log(LOG_INFO, "mic level raw: %s | filtered: %s", raw.toUtf8().constData(),
		formatSnapshot(filtered).toUtf8().constData());
}

void MicLevelProbe::resetMeter(Meter &meter, size_t channels)
{
	std::lock_guard<std::mutex> lock(meter.mutex);
	meter.channels = channels;
	meter.peak = 0.0f;
	meter.sumSquares = 0.0;
	meter.samples = 0;
	meter.packets = 0;
}

MicLevelProbe::Snapshot MicLevelProbe::takeSnapshot(Meter &meter)
{
	std::lock_guard<std::mutex> lock(meter.mutex);
	Snapshot s;
	s.peak = meter.peak;
	s.rms = meter.samples ? std::sqrt(meter.sumSquares / static_cast<double>(meter.samples)) : 0.0;
	s.packets = meter.packets;
	meter.peak = 0.0f;
	meter.sumSquares = 0.0;
	meter.samples = 0;
	meter.packets = 0;
	return s;
}

QString MicLevelProbe::formatSnapshot(const Snapshot &s)
{
	if (s.packets == 0)
		return QStringLiteral("no audio");
	return QStringLiteral("peak %1 dB, rms %2 dB (%3 packets)")
		.arg(format_db(s.peak), format_db(s.rms))
		.arg(s.packets);
}

void MicLevelProbe::onAudio(void *param, obs_source_t *, const struct audio_data *data, bool)
{
	auto *meter = static_cast<Meter *>(param);
	if (!data)
		return;

	// 오디오 스레드에서 호출됨: 계산은 잠금 밖에서 하고 누적만 잠금 안에서 처리
	size_t channels;
	{
		std::lock_guard<std::mutex> lock(meter->mutex);
		channels = meter->channels;
	}

	float peak = 0.0f;
	double sumSquares = 0.0;
	uint64_t samples = 0;
	for (size_t ch = 0; ch < channels && ch < MAX_AV_PLANES; ++ch) {
		const auto *samplesPtr = reinterpret_cast<const float *>(data->data[ch]);
		if (!samplesPtr)
			continue;
		for (uint32_t i = 0; i < data->frames; ++i) {
			const float v = samplesPtr[i];
			const float a = std::fabs(v);
			if (a > peak)
				peak = a;
			sumSquares += static_cast<double>(v) * v;
		}
		samples += data->frames;
	}

	std::lock_guard<std::mutex> lock(meter->mutex);
	if (peak > meter->peak)
		meter->peak = peak;
	meter->sumSquares += sumSquares;
	meter->samples += samples;
	meter->packets++;
}

void MicLevelProbe::onUpdate(void *data, calldata_t *)
{
	// 원본 소스의 장치 설정이 바뀌면 원본 측정용 소스를 새 설정으로 다시 생성
	auto *self = static_cast<MicLevelProbe *>(data);
	QMetaObject::invokeMethod(
		self,
		[self]() {
			if (!self->running_)
				return;
			obs_source_t *source = obs_weak_source_get_source(self->weak_);
			if (!source)
				return;
			self->destroyRaw();
			self->createRaw(source);
			obs_log(LOG_INFO, "mic level probe recreated after source update (raw: %s)",
				self->raw_ ? "ok" : self->rawNote_.toUtf8().constData());
			obs_source_release(source);
		},
		Qt::QueuedConnection);
}
