#include "overlay-window.hpp"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>

namespace {

// 색상 (어두운 반투명 캡슐 + 상태 강조색)
const QColor kPillBackground(24, 24, 27, 225);
const QColor kPillStroke(255, 255, 255, 34);
const QColor kTextPrimary(250, 250, 250);
const QColor kTextSecondary(212, 212, 216);
const QColor kDivider(255, 255, 255, 46);
const QColor kRecRed(239, 68, 68);
const QColor kPauseAmber(245, 158, 11);
const QColor kStopGray(161, 161, 170);
const QColor kMicLive(74, 222, 128);

QFont make_font(int pixelSize, QFont::Weight weight)
{
	QFont font;
	font.setFamilies({QStringLiteral("Segoe UI Variable Text"), QStringLiteral("Segoe UI"),
			  QStringLiteral("Malgun Gothic"), QStringLiteral("Noto Sans CJK KR"),
			  QStringLiteral("sans-serif")});
	font.setPixelSize(pixelSize);
	font.setWeight(weight);
	font.setHintingPreference(QFont::PreferNoHinting);
	return font;
}

void draw_status_icon(QPainter &painter, RecordingState state, const QRectF &box)
{
	painter.setPen(Qt::NoPen);
	const QPointF c = box.center();
	const qreal r = box.width() / 2.0;

	switch (state) {
	case RecordingState::Recording: {
		QColor halo = kRecRed;
		halo.setAlpha(70);
		painter.setBrush(halo);
		painter.drawEllipse(c, r, r);
		painter.setBrush(kRecRed);
		painter.drawEllipse(c, r * 0.62, r * 0.62);
		break;
	}
	case RecordingState::Paused: {
		painter.setBrush(kPauseAmber);
		const qreal barW = r * 0.52;
		const qreal barH = r * 1.5;
		const qreal gap = r * 0.42;
		painter.drawRoundedRect(QRectF(c.x() - gap / 2 - barW, c.y() - barH / 2, barW, barH), barW / 3,
					barW / 3);
		painter.drawRoundedRect(QRectF(c.x() + gap / 2, c.y() - barH / 2, barW, barH), barW / 3, barW / 3);
		break;
	}
	case RecordingState::Stopped: {
		painter.setBrush(kStopGray);
		const qreal s = r * 1.2;
		painter.drawRoundedRect(QRectF(c.x() - s / 2, c.y() - s / 2, s, s), s / 5, s / 5);
		break;
	}
	}
}

void draw_mic_icon(QPainter &painter, const QRectF &box, bool muted)
{
	const qreal w = box.width();
	const qreal h = box.height();
	const QColor color = muted ? kRecRed : kTextPrimary;
	const qreal stroke = qMax<qreal>(1.6, w * 0.10);

	painter.setPen(Qt::NoPen);
	painter.setBrush(color);
	const QRectF capsule(box.left() + w * 0.35, box.top() + h * 0.06, w * 0.30, h * 0.52);
	painter.drawRoundedRect(capsule, capsule.width() / 2, capsule.width() / 2);

	painter.setPen(QPen(color, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	painter.setBrush(Qt::NoBrush);
	const QRectF arc(box.left() + w * 0.20, box.top() + h * 0.20, w * 0.60, h * 0.52);
	painter.drawArc(arc, 180 * 16, 180 * 16);
	const qreal stemTop = arc.bottom();
	const qreal stemBottom = box.top() + h * 0.92;
	painter.drawLine(QPointF(box.center().x(), stemTop), QPointF(box.center().x(), stemBottom));

	if (muted) {
		// 아이콘 뒤 배경색으로 한 번, 강조색으로 한 번 그려 사선이 또렷하게 보이도록 함
		const QPointF a(box.left() + w * 0.12, box.top() + h * 0.08);
		const QPointF b(box.right() - w * 0.12, box.bottom() - h * 0.04);
		painter.setPen(QPen(kPillBackground, stroke * 2.6, Qt::SolidLine, Qt::RoundCap));
		painter.drawLine(a, b);
		painter.setPen(QPen(color, stroke, Qt::SolidLine, Qt::RoundCap));
		painter.drawLine(a, b);
	} else {
		painter.setPen(Qt::NoPen);
		painter.setBrush(kMicLive);
		const qreal d = w * 0.26;
		painter.drawEllipse(QRectF(box.right() - d * 0.9, box.top() + h * 0.02, d, d));
	}
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

void OverlayWindow::setMicState(bool available, bool muted)
{
	if (micAvailable_ == available && micMuted_ == muted)
		return;
	micAvailable_ = available;
	micMuted_ = muted;
	update();
}

void OverlayWindow::setElapsed(qint64 ms)
{
	if (elapsedMs_ / 1000 == ms / 1000) {
		elapsedMs_ = ms;
		return;
	}
	elapsedMs_ = ms;
	if (!settings_.showElapsed || state_ == RecordingState::Stopped)
		return;
	// 캡슐 영역만 다시 그림 (전체 화면 갱신 방지)
	const QRect next = computeLayout().rect;
	update(lastPill_.united(next).adjusted(-16, -16, 16, 16));
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

void OverlayWindow::setTaskbarRect(const QRect &rect)
{
	if (taskbarRect_ == rect)
		return;
	taskbarRect_ = rect;
	update();
}

QString OverlayWindow::statusText() const
{
	switch (state_) {
	case RecordingState::Recording:
		return settings_.textRec;
	case RecordingState::Paused:
		return settings_.textPaused;
	case RecordingState::Stopped:
	default:
		return settings_.textStopped;
	}
}

QString OverlayWindow::elapsedText() const
{
	const qint64 total = elapsedMs_ / 1000;
	const qint64 h = total / 3600;
	const qint64 m = (total / 60) % 60;
	const qint64 s = total % 60;
	return QStringLiteral("%1:%2:%3")
		.arg(h, 2, 10, QLatin1Char('0'))
		.arg(m, 2, 10, QLatin1Char('0'))
		.arg(s, 2, 10, QLatin1Char('0'));
}

QRect OverlayWindow::pillRect(const QSize &size) const
{
	const QRect r = rect();
	const int margin = settings_.borderWidth + 16;
	const int centerX = r.left() + (r.width() - size.width()) / 2;

	switch (settings_.position) {
	case BadgePosition::TaskbarCenter:
		if (!taskbarRect_.isEmpty() && taskbarRect_.width() >= taskbarRect_.height()) {
			// 가로 작업 표시줄: 그 높이 안에서 세로 가운데
			return QRect(QPoint(centerX, taskbarRect_.center().y() - size.height() / 2 + 1), size);
		}
		return QRect(QPoint(centerX, r.bottom() - size.height() - 12 + 1), size);
	case BadgePosition::BottomCenter:
		return QRect(QPoint(centerX, r.bottom() - size.height() - margin + 1), size);
	case BadgePosition::TopLeft:
		return QRect(QPoint(r.left() + margin, r.top() + margin), size);
	case BadgePosition::TopCenter:
		return QRect(QPoint(centerX, r.top() + margin), size);
	case BadgePosition::TopRight:
		return QRect(QPoint(r.right() - size.width() - margin + 1, r.top() + margin), size);
	case BadgePosition::BottomLeft:
		return QRect(QPoint(r.left() + margin, r.bottom() - size.height() - margin + 1), size);
	case BadgePosition::BottomRight:
		return QRect(QPoint(r.right() - size.width() - margin + 1, r.bottom() - size.height() - margin + 1),
			     size);
	}
	return QRect(QPoint(centerX, r.bottom() - size.height() - margin + 1), size);
}

OverlayWindow::PillLayout OverlayWindow::computeLayout() const
{
	PillLayout layout;
	if (!settings_.enabled)
		return layout;

	switch (state_) {
	case RecordingState::Recording:
		layout.showStatus = settings_.showRec;
		break;
	case RecordingState::Paused:
		layout.showStatus = true;
		break;
	case RecordingState::Stopped:
		layout.showStatus = settings_.showStopped;
		break;
	}
	layout.showTime = layout.showStatus && settings_.showElapsed && state_ != RecordingState::Stopped;
	layout.showMic = micAvailable_;
	if (!layout.showStatus && !layout.showMic)
		return layout;

	int fontPx = qMax(10, settings_.fontSize);
	if (settings_.position == BadgePosition::TaskbarCenter && !taskbarRect_.isEmpty() &&
	    taskbarRect_.width() >= taskbarRect_.height()) {
		// 캡슐 높이(글자 × 2.2)가 작업 표시줄 높이 - 10px 안에 들어오도록 축소
		const int maxFont = static_cast<int>((taskbarRect_.height() - 10) / 2.2);
		fontPx = qBound(10, maxFont, fontPx);
	}
	layout.fontPx = fontPx;

	const QFontMetrics label(make_font(fontPx, QFont::DemiBold));
	const QFontMetrics mono(make_font(fontPx, QFont::Medium));
	const int height = qRound(fontPx * 2.2);
	const int padX = qRound(fontPx * 0.95);
	const int gap = qRound(fontPx * 0.55);
	const int icon = qRound(fontPx * 1.05);

	int width = padX * 2;
	int parts = 0;
	if (layout.showStatus) {
		width += icon + gap + label.horizontalAdvance(statusText());
		++parts;
	}
	if (layout.showTime)
		width += gap + mono.horizontalAdvance(QStringLiteral("00:00:00"));
	if (layout.showMic) {
		if (parts > 0)
			width += gap * 2 + 1;
		width += icon;
	}
	layout.rect = pillRect(QSize(width, height));
	return layout;
}

void OverlayWindow::drawGlow(QPainter &painter, const QColor &color) const
{
	const int bw = settings_.borderWidth;
	if (bw <= 0)
		return;
	// 바깥쪽이 진하고 안쪽으로 옅어지는 테두리
	painter.setBrush(Qt::NoBrush);
	for (int i = 0; i < bw; ++i) {
		const qreal t = 1.0 - static_cast<qreal>(i) / bw;
		QColor c = color;
		c.setAlphaF(0.85 * t * t);
		painter.setPen(QPen(c, 1.0));
		painter.drawRect(QRectF(rect()).adjusted(i + 0.5, i + 0.5, -i - 0.5, -i - 0.5));
	}
}

void OverlayWindow::drawPill(QPainter &painter, const PillLayout &layout) const
{
	const QRectF pill = layout.rect;
	const qreal radius = pill.height() / 2.0;
	const int fontPx = layout.fontPx;

	// 부드러운 그림자
	painter.setPen(Qt::NoPen);
	for (int i = 6; i >= 1; --i) {
		painter.setBrush(QColor(0, 0, 0, 10));
		painter.drawRoundedRect(pill.adjusted(-i, -i + 2, i, i + 2), radius + i, radius + i);
	}

	QColor accent;
	switch (state_) {
	case RecordingState::Recording:
		accent = kRecRed;
		break;
	case RecordingState::Paused:
		accent = kPauseAmber;
		break;
	case RecordingState::Stopped:
		accent = kStopGray;
		break;
	}

	painter.setBrush(kPillBackground);
	QColor stroke = kPillStroke;
	if (layout.showStatus && state_ == RecordingState::Paused)
		stroke = QColor(accent.red(), accent.green(), accent.blue(), 150);
	painter.setPen(QPen(stroke, 1.0));
	painter.drawRoundedRect(pill.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

	const int padX = qRound(fontPx * 0.95);
	const int gap = qRound(fontPx * 0.55);
	const int icon = qRound(fontPx * 1.05);
	const qreal cy = pill.center().y();
	qreal x = pill.left() + padX;

	if (layout.showStatus) {
		draw_status_icon(painter, state_, QRectF(x, cy - icon / 2.0, icon, icon));
		x += icon + gap;

		const QFont font = make_font(fontPx, QFont::DemiBold);
		painter.setFont(font);
		painter.setPen(state_ == RecordingState::Stopped ? kTextSecondary : kTextPrimary);
		const QString text = statusText();
		const int w = QFontMetrics(font).horizontalAdvance(text);
		painter.drawText(QRectF(x, pill.top(), w + 2, pill.height()), Qt::AlignVCenter | Qt::AlignLeft, text);
		x += w;
	}

	if (layout.showTime) {
		x += gap;
		QFont font = make_font(fontPx, QFont::Medium);
		font.setStyleHint(QFont::Monospace);
		painter.setFont(font);
		painter.setPen(state_ == RecordingState::Paused ? kPauseAmber : kTextSecondary);
		const int w = QFontMetrics(font).horizontalAdvance(QStringLiteral("00:00:00"));
		painter.drawText(QRectF(x, pill.top(), w + 2, pill.height()), Qt::AlignVCenter | Qt::AlignLeft,
				 elapsedText());
		x += w;
	}

	if (layout.showMic) {
		if (layout.showStatus) {
			x += gap;
			painter.setPen(QPen(kDivider, 1.0));
			const qreal dh = pill.height() * 0.42;
			painter.drawLine(QPointF(x + 0.5, cy - dh / 2), QPointF(x + 0.5, cy + dh / 2));
			x += 1 + gap;
		}
		draw_mic_icon(painter, QRectF(x, cy - icon / 2.0, icon, icon), micMuted_);
	}
}

void OverlayWindow::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(rect(), Qt::transparent);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	if (!settings_.enabled) {
		lastPill_ = QRect();
		return;
	}

	// 일시정지만 화면 테두리로 강조 (재개 누락 방지), 나머지 상태는 캡슐만 표시
	if (state_ == RecordingState::Paused)
		drawGlow(painter, kPauseAmber);

	const PillLayout layout = computeLayout();
	lastPill_ = layout.rect;
	if (layout.rect.isEmpty())
		return;
	drawPill(painter, layout);
}
