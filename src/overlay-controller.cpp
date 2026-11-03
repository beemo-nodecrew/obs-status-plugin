#include "overlay-controller.hpp"
#include "overlay-window.hpp"

#include <plugin-support.h>
#include <obs-module.h>

#include <QGuiApplication>
#include <QScreen>

OverlayController::OverlayController(QObject *parent) : QObject(parent)
{
	connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *) { placeWindow(); });
	connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) { placeWindow(); });
}

OverlayController::~OverlayController()
{
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
}

void OverlayController::setRecordingState(RecordingState state)
{
	state_ = state;
	if (window_)
		window_->setRecordingState(state);
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
	if (!window_->isVisible())
		window_->show();
	window_->moveToScreen(screen);

	const QRect g = screen->geometry();
	obs_log(LOG_INFO, "overlay placed on screen '%s' (%d,%d %dx%d)", screen->name().toUtf8().constData(), g.x(),
		g.y(), g.width(), g.height());
}
