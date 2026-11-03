#include "overlay-controller.hpp"
#include "overlay-window.hpp"
#include "settings-dialog.hpp"
#include "settings-store.hpp"

#include <plugin-support.h>
#include <obs-module.h>

#include <QGuiApplication>
#include <QScreen>

OverlayController::OverlayController(QObject *parent) : QObject(parent), settings_(load_overlay_settings())
{
	connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *) { placeWindow(); });
	connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) { placeWindow(); });

	topmostTimer_.setInterval(1500);
	connect(&topmostTimer_, &QTimer::timeout, this, [this]() {
		if (window_ && window_->isVisible())
			raise_topmost(window_);
	});
}

OverlayController::~OverlayController()
{
	topmostTimer_.stop();
	delete dialog_;
	delete window_;
}

void OverlayController::start()
{
	if (!window_) {
		window_ = new OverlayWindow();
		window_->applySettings(settings_);
		window_->setRecordingState(state_);
	}
	placeWindow();
	topmostTimer_.start();
}

void OverlayController::setRecordingState(RecordingState state)
{
	state_ = state;
	if (window_)
		window_->setRecordingState(state);
}

void OverlayController::openSettings(QWidget *parent)
{
	if (!dialog_) {
		dialog_ = new SettingsDialog(parent);
		connect(dialog_, &SettingsDialog::settingsApplied, this, [this](const OverlaySettings &settings) {
			applySettings(settings);
			save_overlay_settings(settings_);
		});
	}
	dialog_->load(settings_, exclusion_);
	dialog_->show();
	dialog_->raise();
	dialog_->activateWindow();
}

void OverlayController::applySettings(const OverlaySettings &settings)
{
	settings_ = settings;
	if (window_) {
		window_->applySettings(settings_);
		placeWindow();
	}
}

QScreen *OverlayController::targetScreen() const
{
	const QList<QScreen *> screens = QGuiApplication::screens();
	if (screens.isEmpty())
		return nullptr;

	if (!settings_.screenName.isEmpty()) {
		for (QScreen *screen : screens) {
			if (screen->name() == settings_.screenName)
				return screen;
		}
	}
	if (settings_.screenIndex >= 0 && settings_.screenIndex < screens.size())
		return screens.at(settings_.screenIndex);
	return QGuiApplication::primaryScreen();
}

void OverlayController::placeWindow()
{
	if (!window_)
		return;

	QScreen *screen = targetScreen();
	if (!screen) {
		window_->hide();
		return;
	}

	connect(screen, &QScreen::geometryChanged, this, &OverlayController::placeWindow, Qt::UniqueConnection);

	window_->moveToScreen(screen);
	if (!window_->isVisible()) {
		// 표시 전에 캡처 제외를 걸어 녹화본에 한 프레임도 남지 않게 함
		applyCaptureExclusion();
		window_->show();
	}
	window_->moveToScreen(screen);
	applyCaptureExclusion();
	raise_topmost(window_);

	const QRect g = screen->geometry();
	obs_log(LOG_INFO, "overlay placed on screen '%s' (%d,%d %dx%d)", screen->name().toUtf8().constData(), g.x(),
		g.y(), g.width(), g.height());
}

void OverlayController::applyCaptureExclusion()
{
	if (!window_)
		return;
	const CaptureExclusionResult result = exclude_from_capture(window_);
	const bool changed = result.success != exclusion_.success || result.message != exclusion_.message;
	exclusion_ = result;
	if (!changed)
		return;
	if (result.success)
		obs_log(LOG_INFO, "capture exclusion applied: %s", result.message.toUtf8().constData());
	else
		obs_log(LOG_WARNING, "capture exclusion unavailable: %s", result.message.toUtf8().constData());
}
