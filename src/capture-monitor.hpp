#pragma once

#include "source-label.hpp"

#include <obs.h>

#include <QObject>
#include <QString>

// 설정에서 지정한 디스플레이 캡처 소스가 지금 캡처 중인 모니터 이름 추적
class CaptureMonitor : public QObject {
	Q_OBJECT

public:
	explicit CaptureMonitor(QObject *parent = nullptr);
	~CaptureMonitor() override;

	void setSourceName(const QString &name);
	const QString &sourceName() const { return name_; }
	SourceLabel label() const { return label_; }
	void refresh(); // 다시 읽고, 바뀌었으면 labelChanged

signals:
	void labelChanged();
	void sourceRenamed(const QString &newName);

private:
	static void onSourceRename(void *data, calldata_t *cd);

	QString name_;
	SourceLabel label_;
};
