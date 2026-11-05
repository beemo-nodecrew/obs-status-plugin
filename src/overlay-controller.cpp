#include "overlay-controller.hpp"
#include "mic-monitor.hpp"
#include "overlay-window.hpp"
#include "settings-dialog.hpp"
#include "settings-store.hpp"

#include <plugin-support.h>
#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QGuiApplication>
#include <QScreen>

OverlayController::OverlayController(QObject *parent) : QObject(parent), settings_(load_overlay_settings())
{
	connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *) { placeWindow(); });
	connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) { placeWindow(); });

	mic_ = new MicMonitor(this);
	connect(mic_, &MicMonitor::mutedChanged, this, [this](bool) { updateMicState(); });
	connect(mic_, &MicMonitor::attachedChanged, this, [this](bool) { updateMicState(); });
	connect(mic_, &MicMonitor::sourceRenamed, this, [this](const QString &name) {
		settings_.micSource = name;
		save_overlay_settings(settings_);
	});

	tickTimer_.setInterval(250);
	connect(&tickTimer_, &QTimer::timeout, this, [this]() {
		if (window_)
			window_->setElapsed(elapsedMs());
	});

	topmostTimer_.setInterval(1500);
	connect(&topmostTimer_, &QTimer::timeout, this, [this]() {
		if (window_ && window_->isVisible())
			raise_topmost(window_);
	});
}

OverlayController::~OverlayController()
{
	topmostTimer_.stop();
	tickTimer_.stop();
	delete mic_;
	mic_ = nullptr;
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
	mic_->setSourceName(settings_.micSource);
	mic_->setLevelLogEnabled(settings_.micLevelLog);
	updateMicState();
	if (state_ != RecordingState::Stopped)
		initElapsedFromOutput();
	placeWindow();
	topmostTimer_.start();
	tickTimer_.start();
}

void OverlayController::refreshMic()
{
	mic_->refresh();
	updateMicState();
}

void OverlayController::updateMicState()
{
	if (window_ && mic_)
		window_->setMicState(mic_->isAttached(), mic_->isMuted());
}

qint64 OverlayController::elapsedMs() const
{
	return accumulatedMs_ + (runTimer_.isValid() ? runTimer_.elapsed() : 0);
}

void OverlayController::initElapsedFromOutput()
{
	// 플러그인보다 먼저 녹화가 시작된 경우: 출력된 프레임 수로 경과 시간 추정
	accumulatedMs_ = 0;
	runTimer_.invalidate();
	obs_output_t *output = obs_frontend_get_recording_output();
	if (output) {
		struct obs_video_info ovi = {};
		const int frames = obs_output_get_total_frames(output);
		if (obs_get_video_info(&ovi) && ovi.fps_num > 0)
			accumulatedMs_ = static_cast<qint64>(frames) * 1000 * ovi.fps_den / ovi.fps_num;
		obs_output_release(output);
	}
	if (state_ == RecordingState::Recording)
		runTimer_.start();
}

void OverlayController::setRecordingState(RecordingState state)
{
	const RecordingState prev = state_;
	state_ = state;

	if (state == RecordingState::Stopped) {
		accumulatedMs_ = 0;
		runTimer_.invalidate();
	} else if (prev == RecordingState::Stopped && state == RecordingState::Recording) {
		accumulatedMs_ = 0;
		runTimer_.start();
	} else if (prev == RecordingState::Recording && state == RecordingState::Paused) {
		accumulatedMs_ = elapsedMs();
		runTimer_.invalidate();
	} else if (prev == RecordingState::Paused && state == RecordingState::Recording) {
		runTimer_.start();
	}

	if (window_) {
		window_->setElapsed(elapsedMs());
		window_->setRecordingState(state);
	}
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
	mic_->setSourceName(settings_.micSource);
	mic_->setLevelLogEnabled(settings_.micLevelLog);
	if (window_) {
		updateMicState();
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
	connect(screen, &QScreen::availableGeometryChanged, this, &OverlayController::placeWindow,
		Qt::UniqueConnection);

	window_->moveToScreen(screen);
	if (!window_->isVisible()) {
		// 표시 전에 캡처 제외를 걸어 녹화본에 한 프레임도 남지 않게 함
		applyCaptureExclusion();
		window_->show();
	}
	window_->moveToScreen(screen);
	updateTaskbarRect(screen);
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

void OverlayController::updateTaskbarRect(QScreen *screen)
{
	if (!window_ || !screen)
		return;
	// 작업 영역(availableGeometry) 밖의 아래쪽 띠 = 하단 작업 표시줄
	const QRect full = screen->geometry();
	const QRect avail = screen->availableGeometry();
	QRect taskbar;
	if (avail.bottom() < full.bottom())
		taskbar = QRect(0, avail.bottom() + 1 - full.top(), full.width(), full.bottom() - avail.bottom());
	window_->setTaskbarRect(taskbar);
}
