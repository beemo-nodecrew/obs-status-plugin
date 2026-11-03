#pragma once

#include <obs.h>

#include <QObject>
#include <QString>

class MicLevelProbe;

// 지정한 오디오 소스의 음소거 상태 추적 (이름 변경·삭제·재생성 대응)
class MicMonitor : public QObject {
	Q_OBJECT

public:
	explicit MicMonitor(QObject *parent = nullptr);
	~MicMonitor() override;

	void setSourceName(const QString &name);
	const QString &sourceName() const { return name_; }
	bool isMuted() const { return muted_; }
	void refresh();
	void setLevelLogEnabled(bool enabled);

signals:
	void mutedChanged(bool muted);
	void sourceRenamed(const QString &newName);

private:
	void attach();
	void detach();
	void setMuted(bool muted);

	static void onMute(void *data, calldata_t *cd);
	static void onSourceCreate(void *data, calldata_t *cd);
	static void onSourceRename(void *data, calldata_t *cd);
	static void onSourceGone(void *data, calldata_t *cd);

	QString name_;
	obs_weak_source_t *weak_ = nullptr;
	obs_source_t *attached_ = nullptr; // 비교 전용, 역참조 금지
	bool muted_ = false;
	MicLevelProbe *probe_ = nullptr;
};
