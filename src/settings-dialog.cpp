#include "settings-dialog.hpp"

#include <obs-module.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QString tr_text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

bool collect_audio_source(void *param, obs_source_t *source)
{
	auto *names = static_cast<QStringList *>(param);
	if (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) {
		const char *name = obs_source_get_name(source);
		if (name && *name)
			names->append(QString::fromUtf8(name));
	}
	return true;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr_text("Settings.Title"));
	setAttribute(Qt::WA_DeleteOnClose, false);

	enabled_ = new QCheckBox(tr_text("Settings.Enabled"), this);
	screen_ = new QComboBox(this);
	mic_ = new QComboBox(this);
	micLevelLog_ = new QCheckBox(tr_text("Settings.MicLevelLog"), this);
	position_ = new QComboBox(this);
	position_->addItem(tr_text("Position.TaskbarCenter"), static_cast<int>(BadgePosition::TaskbarCenter));
	position_->addItem(tr_text("Position.BottomCenter"), static_cast<int>(BadgePosition::BottomCenter));
	position_->addItem(tr_text("Position.TopLeft"), static_cast<int>(BadgePosition::TopLeft));
	position_->addItem(tr_text("Position.TopCenter"), static_cast<int>(BadgePosition::TopCenter));
	position_->addItem(tr_text("Position.TopRight"), static_cast<int>(BadgePosition::TopRight));
	position_->addItem(tr_text("Position.BottomLeft"), static_cast<int>(BadgePosition::BottomLeft));
	position_->addItem(tr_text("Position.BottomRight"), static_cast<int>(BadgePosition::BottomRight));

	fontSize_ = new QSpinBox(this);
	fontSize_->setRange(10, 200);
	fontSize_->setSuffix(QStringLiteral(" px"));
	borderWidth_ = new QSpinBox(this);
	borderWidth_->setRange(0, 100);
	borderWidth_->setSuffix(QStringLiteral(" px"));

	showRec_ = new QCheckBox(tr_text("Settings.ShowRec"), this);
	showStopped_ = new QCheckBox(tr_text("Settings.ShowStopped"), this);
	showElapsed_ = new QCheckBox(tr_text("Settings.ShowElapsed"), this);
	textPaused_ = new QLineEdit(this);
	textStopped_ = new QLineEdit(this);
	textRec_ = new QLineEdit(this);
	exclusion_ = new QLabel(this);
	exclusion_->setWordWrap(true);

	auto *form = new QFormLayout();
	form->addRow(enabled_);
	form->addRow(tr_text("Settings.Screen"), screen_);
	form->addRow(tr_text("Settings.MicSource"), mic_);
	form->addRow(micLevelLog_);
	form->addRow(tr_text("Settings.Position"), position_);
	form->addRow(tr_text("Settings.FontSize"), fontSize_);
	form->addRow(tr_text("Settings.BorderWidth"), borderWidth_);
	form->addRow(showRec_);
	form->addRow(showStopped_);
	form->addRow(showElapsed_);
	form->addRow(tr_text("Settings.TextPaused"), textPaused_);
	form->addRow(tr_text("Settings.TextStopped"), textStopped_);
	form->addRow(tr_text("Settings.TextRec"), textRec_);
	form->addRow(tr_text("Settings.CaptureExclusion"), exclusion_);

	auto *buttons =
		new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
		emit settingsApplied(collect());
		accept();
	});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
		[this]() { emit settingsApplied(collect()); });

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);
}

void SettingsDialog::load(const OverlaySettings &settings, const CaptureExclusionResult &exclusion)
{
	base_ = settings;
	enabled_->setChecked(settings.enabled);
	populateScreens(settings);
	populateMicSources(settings.micSource);
	micLevelLog_->setChecked(settings.micLevelLog);
	position_->setCurrentIndex(qMax(0, position_->findData(static_cast<int>(settings.position))));
	fontSize_->setValue(settings.fontSize);
	borderWidth_->setValue(settings.borderWidth);
	showRec_->setChecked(settings.showRec);
	showStopped_->setChecked(settings.showStopped);
	showElapsed_->setChecked(settings.showElapsed);
	textPaused_->setText(settings.textPaused);
	textStopped_->setText(settings.textStopped);
	textRec_->setText(settings.textRec);
	updateExclusionLabel(exclusion);
}

OverlaySettings SettingsDialog::collect() const
{
	OverlaySettings s = base_;
	s.enabled = enabled_->isChecked();
	const int screenIndex = screen_->currentIndex();
	if (screenIndex >= 0) {
		s.screenIndex = screenIndex;
		s.screenName = screen_->currentData().toString();
	}
	s.micSource = mic_->currentIndex() > 0 ? mic_->currentData().toString() : QString();
	s.micLevelLog = micLevelLog_->isChecked();
	s.position = static_cast<BadgePosition>(position_->currentData().toInt());
	s.fontSize = fontSize_->value();
	s.borderWidth = borderWidth_->value();
	s.showRec = showRec_->isChecked();
	s.showStopped = showStopped_->isChecked();
	s.showElapsed = showElapsed_->isChecked();
	s.textPaused = textPaused_->text().trimmed();
	s.textStopped = textStopped_->text().trimmed();
	s.textRec = textRec_->text().trimmed();
	return s;
}

void SettingsDialog::populateScreens(const OverlaySettings &settings)
{
	screen_->clear();
	const QList<QScreen *> screens = QGuiApplication::screens();
	int selected = -1;
	for (int i = 0; i < screens.size(); ++i) {
		QScreen *screen = screens.at(i);
		const QRect g = screen->geometry();
		QString label = QStringLiteral("%1: %2 (%3x%4, %5,%6)")
					.arg(i + 1)
					.arg(screen->name())
					.arg(g.width())
					.arg(g.height())
					.arg(g.x())
					.arg(g.y());
		if (screen == QGuiApplication::primaryScreen())
			label += QStringLiteral(" - ") + tr_text("Settings.Primary");
		screen_->addItem(label, screen->name());
		if (selected < 0 && !settings.screenName.isEmpty() && screen->name() == settings.screenName)
			selected = i;
	}
	if (selected < 0 && settings.screenIndex >= 0 && settings.screenIndex < screens.size())
		selected = settings.screenIndex;
	screen_->setCurrentIndex(qMax(0, selected));
}

void SettingsDialog::populateMicSources(const QString &selected)
{
	mic_->clear();
	mic_->addItem(tr_text("Settings.MicNone"), QString());

	QStringList names;
	obs_enum_sources(collect_audio_source, &names);
	names.removeDuplicates();
	names.sort(Qt::CaseInsensitive);
	if (!selected.isEmpty() && !names.contains(selected))
		names.prepend(selected);

	for (const QString &name : names)
		mic_->addItem(name, name);

	const int index = selected.isEmpty() ? 0 : mic_->findData(selected);
	mic_->setCurrentIndex(qMax(0, index));
}

void SettingsDialog::updateExclusionLabel(const CaptureExclusionResult &exclusion)
{
	if (exclusion.success) {
		exclusion_->setText(tr_text("Settings.CaptureExclusion.Ok"));
		exclusion_->setStyleSheet(QString());
		return;
	}
	exclusion_->setText(tr_text("Settings.CaptureExclusion.Failed") + QStringLiteral("\n") + exclusion.message);
	exclusion_->setStyleSheet(QStringLiteral("color: #dc2626;"));
}
