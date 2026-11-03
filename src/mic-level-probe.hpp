#pragma once

#include <obs.h>

#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <mutex>

// 진단용: 마이크 소스의 필터 후 신호와, 같은 장치를 필터 없이 연 원본 신호의 크기를 1초마다 로그로 남김
// 송신기 OFF와 '켜져 있지만 말 안 함'을 구분할 판단 기준을 찾기 위한 측정 도구
class MicLevelProbe : public QObject {
	Q_OBJECT

public:
	explicit MicLevelProbe(QObject *parent = nullptr);
	~MicLevelProbe() override;

	void setEnabled(bool enabled);
	void setSource(obs_source_t *source); // nullptr이면 측정 대상 없음

private:
	struct Meter {
		std::mutex mutex;
		size_t channels = 0;
		float peak = 0.0f;
		double sumSquares = 0.0;
		uint64_t samples = 0;
		uint64_t packets = 0;
	};

	struct Snapshot {
		float peak = 0.0f;
		double rms = 0.0;
		uint64_t packets = 0;
	};

	void start();
	void stop();
	void createRaw(obs_source_t *source);
	void destroyRaw();
	void report();

	static void resetMeter(Meter &meter, size_t channels);
	static Snapshot takeSnapshot(Meter &meter);
	static QString formatSnapshot(const Snapshot &snapshot);
	static void onAudio(void *param, obs_source_t *source, const struct audio_data *data, bool muted);
	static void onUpdate(void *data, calldata_t *cd);

	bool enabled_ = false;
	bool running_ = false;
	obs_weak_source_t *weak_ = nullptr;
	obs_source_t *raw_ = nullptr;
	QString rawNote_;
	Meter filteredMeter_;
	Meter rawMeter_;
	QTimer timer_;
};
