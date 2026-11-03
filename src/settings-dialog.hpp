#pragma once

#include "overlay-settings.hpp"
#include "window-utils.hpp"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = nullptr);

	void load(const OverlaySettings &settings, const CaptureExclusionResult &exclusion);
	OverlaySettings collect() const;

signals:
	void settingsApplied(const OverlaySettings &settings);

private:
	void populateScreens(const OverlaySettings &settings);
	void populateMicSources(const QString &selected);
	void updateExclusionLabel(const CaptureExclusionResult &exclusion);

	OverlaySettings base_;
	QCheckBox *enabled_ = nullptr;
	QComboBox *screen_ = nullptr;
	QComboBox *mic_ = nullptr;
	QComboBox *position_ = nullptr;
	QSpinBox *fontSize_ = nullptr;
	QSpinBox *borderWidth_ = nullptr;
	QCheckBox *showRec_ = nullptr;
	QCheckBox *showStopped_ = nullptr;
	QLineEdit *textPaused_ = nullptr;
	QLineEdit *textStopped_ = nullptr;
	QLineEdit *textRec_ = nullptr;
	QLabel *exclusion_ = nullptr;
};
