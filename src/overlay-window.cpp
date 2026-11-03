#include "overlay-window.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>

namespace {

const QColor kPausedColor(220, 38, 38);
const QColor kStoppedColor(250, 204, 21);
const QColor kBadgeTextLight(255, 255, 255);
const QColor kBadgeTextDark(23, 23, 23);
const QColor kRecBackground(0, 0, 0, 150);

// 음소거된 마이크 아이콘 (캡슐 + 받침 + 사선)
void drawMutedMic(QPainter &painter, const QRectF &box, const QColor &color)
{
	const qreal w = box.width();
	const qreal h = box.height();
	const qreal stroke = qMax<qreal>(2.0, w * 0.09);

	QPen pen(color, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	const QRectF capsule(box.left() + w * 0.34, box.top() + h * 0.08, w * 0.32, h * 0.50);
	painter.drawRoundedRect(capsule, capsule.width() / 2, capsule.width() / 2);

	const QRectF arc(box.left() + w * 0.20, box.top() + h * 0.22, w * 0.60, h * 0.50);
	painter.drawArc(arc, 180 * 16, 180 * 16);

	const QPointF stemTop(box.center().x(), arc.bottom());
	const QPointF stemBottom(box.center().x(), box.top() + h * 0.88);
	painter.drawLine(stemTop, stemBottom);
	painter.drawLine(QPointF(box.left() + w * 0.32, stemBottom.y()),
			 QPointF(box.left() + w * 0.68, stemBottom.y()));

	QPen slash(kPausedColor, stroke * 1.2, Qt::SolidLine, Qt::RoundCap);
	painter.setPen(slash);
	painter.drawLine(QPointF(box.left() + w * 0.15, box.top() + h * 0.10),
			 QPointF(box.right() - w * 0.15, box.bottom() - h * 0.06));
}

} // namespace

OverlayWindow::OverlayWindow(QWidget *parent) : QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowTransparentForInput |
		       Qt::WindowDoesNotAcceptFocus);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_MacAlwaysShowToolWindow);
	setFocusPolicy(Qt::NoFocus);
	setWindowTitle(QStringLiteral("OBS Status Overlay"));
}

void OverlayWindow::setRecordingState(RecordingState state)
{
	if (state_ == state)
		return;
	state_ = state;
	update();
}

void OverlayWindow::setMicMuted(bool muted)
{
	if (micMuted_ == muted)
		return;
	micMuted_ = muted;
	update();
}

void OverlayWindow::applySettings(const OverlaySettings &settings)
{
	settings_ = settings;
	update();
}

void OverlayWindow::moveToScreen(QScreen *screen)
{
	if (!screen)
		return;
	if (QWindow *handle = windowHandle())
		handle->setScreen(screen);
	setGeometry(screen->geometry());
	update();
}

QRect OverlayWindow::badgeRect(const QSize &size) const
{
	const int margin = settings_.borderWidth + qMax(12, settings_.fontSize / 2);
	const QRect area = rect().adjusted(margin, margin, -margin, -margin);
	int x = area.left();
	int y = area.top();

	switch (settings_.position) {
	case BadgePosition::TopLeft:
		break;
	case BadgePosition::TopCenter:
		x = area.left() + (area.width() - size.width()) / 2;
		break;
	case BadgePosition::TopRight:
		x = area.right() - size.width() + 1;
		break;
	case BadgePosition::BottomLeft:
		y = area.bottom() - size.height() + 1;
		break;
	case BadgePosition::BottomRight:
		x = area.right() - size.width() + 1;
		y = area.bottom() - size.height() + 1;
		break;
	}
	return QRect(QPoint(x, y), size);
}

void OverlayWindow::drawBorder(QPainter &painter, const QColor &color)
{
	const int bw = settings_.borderWidth;
	if (bw <= 0)
		return;
	QPen pen(color, bw);
	pen.setJoinStyle(Qt::MiterJoin);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	const qreal half = bw / 2.0;
	painter.drawRect(QRectF(rect()).adjusted(half, half, -half, -half));
}

void OverlayWindow::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(rect(), Qt::transparent);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	if (!settings_.enabled)
		return;

	QString text;
	QColor background;
	QColor foreground = kBadgeTextLight;
	bool recDot = false;

	switch (state_) {
	case RecordingState::Paused:
		drawBorder(painter, kPausedColor);
		text = settings_.textPaused;
		background = kPausedColor;
		break;
	case RecordingState::Stopped:
		if (settings_.showStopped) {
			drawBorder(painter, kStoppedColor);
			text = settings_.textStopped;
			background = kStoppedColor;
			foreground = kBadgeTextDark;
		}
		break;
	case RecordingState::Recording:
		if (settings_.showRec) {
			text = settings_.textRec;
			background = kRecBackground;
			recDot = true;
		}
		break;
	}

	const bool showMic = micMuted_;
	if (text.isEmpty() && !showMic)
		return;

	QFont font = this->font();
	font.setPixelSize(qMax(10, settings_.fontSize));
	font.setBold(true);
	painter.setFont(font);
	const QFontMetrics fm(font);

	const int padX = fm.height() / 2;
	const int padY = fm.height() / 4;
	const int gap = fm.height() / 3;
	const int iconSize = fm.height();
	const int dotSize = fm.height() / 2;

	int contentWidth = 0;
	if (recDot)
		contentWidth += dotSize + gap;
	if (!text.isEmpty())
		contentWidth += fm.horizontalAdvance(text);
	if (showMic)
		contentWidth += (contentWidth > 0 ? gap : 0) + iconSize;

	if (text.isEmpty()) {
		background = kRecBackground;
		foreground = kBadgeTextLight;
	}

	const QSize badgeSize(contentWidth + padX * 2, fm.height() + padY * 2);
	const QRect badge = badgeRect(badgeSize);

	painter.setPen(Qt::NoPen);
	painter.setBrush(background);
	painter.drawRoundedRect(badge, badge.height() / 4.0, badge.height() / 4.0);

	int x = badge.left() + padX;
	const int centerY = badge.center().y();

	if (recDot) {
		painter.setBrush(kPausedColor);
		painter.drawEllipse(QRect(x, centerY - dotSize / 2, dotSize, dotSize));
		x += dotSize + gap;
	}
	if (!text.isEmpty()) {
		painter.setPen(foreground);
		const int width = fm.horizontalAdvance(text);
		painter.drawText(QRect(x, badge.top() + padY, width, fm.height()), Qt::AlignVCenter | Qt::AlignLeft,
				 text);
		x += width + gap;
	}
	if (showMic)
		drawMutedMic(painter, QRectF(x, centerY - iconSize / 2.0, iconSize, iconSize), foreground);
}
