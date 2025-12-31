#include "app/widgets/pulse_vu_meter.hpp"
#include "app/services/alsa_level.hpp"

#include <QPainter>
#include <QTimer>
#include <QtMath>

// ---- UI widget (LED bar) ----

PulseVUMeter::PulseVUMeter(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("PulseVUMeter");
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    // 20 FPS redraw + decay = smooth enough, cheap enough
    tick_ = new QTimer(this);
    tick_->setInterval(50);
    connect(tick_, &QTimer::timeout, this, [this]{

const float tL = levelL_.load(std::memory_order_relaxed);
const float tR = levelR_.load(std::memory_order_relaxed);

// smoothing (fast rise, slow fall)
auto smooth = [](float target, float current){
    if (target > current) return current * 0.65f + target * 0.35f;
    return current * 0.92f + target * 0.08f;
};

displayL_ = smooth(tL, displayL_);
displayR_ = smooth(tR, displayR_);

peakHoldL_ = qMax(peakHoldL_ * 0.97f, displayL_);
peakHoldR_ = qMax(peakHoldR_ * 0.97f, displayR_);


        update();
    });
    tick_->start();
    // Start PulseAudio monitor reader
pulse_ = new AlsaLevel(this);
connect(pulse_, &AlsaLevel::level, this, &PulseVUMeter::setLevel, Qt::QueuedConnection);
pulse_->start();

}

PulseVUMeter::~PulseVUMeter()
{
    if (pulse_) pulse_->stop();
}

void PulseVUMeter::setLevel(float left, float right)
{
    left = qBound(0.f, left, 1.f);
    right = qBound(0.f, right, 1.f);
    levelL_.store(left, std::memory_order_relaxed);
    levelR_.store(right, std::memory_order_relaxed);
}


static QColor lerp(const QColor &a, const QColor &b, float t)
{
    t = qBound(0.f, t, 1.f);
    return QColor(
        a.red()   + (b.red()   - a.red())   * t,
        a.green() + (b.green() - a.green()) * t,
        a.blue()  + (b.blue()  - a.blue())  * t,
        a.alpha() + (b.alpha() - a.alpha()) * t
    );
}

void PulseVUMeter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

const QRect outer = rect().adjusted(8, 6, -8, -6);

// Two columns with a small gap
const int gapCols = 6;
const int colW = (outer.width() - gapCols) / 2;

QRect rL(outer.left(), outer.top(), colW, outer.height());
QRect rR(outer.left() + colW + gapCols, outer.top(), colW, outer.height());

// LED segments
const int segments = 14;
const int gap = 3;
const int segH = (outer.height() - gap * (segments - 1)) / segments;

// theme gradient: low cyan -> mid purple -> high red
const QColor low( 60, 230, 255, 220);
const QColor mid(160,  80, 255, 220);
const QColor high(255,  80,  80, 230);

auto lerp = [](const QColor &a, const QColor &b, float t){
    t = qBound(0.f, t, 1.f);
    return QColor(
        a.red()   + (b.red()   - a.red())   * t,
        a.green() + (b.green() - a.green()) * t,
        a.blue()  + (b.blue()  - a.blue())  * t,
        a.alpha() + (b.alpha() - a.alpha()) * t
    );
};

auto drawColumn = [&](const QRect &r, float v, float peakHold){
    v = qBound(0.f, v, 1.f);
    const int lit = qRound(v * segments);

    for (int i = 0; i < segments; ++i) {
        const int idxFromBottom = i;
        const int y = r.bottom() - (idxFromBottom + 1) * segH - idxFromBottom * gap;

        QRect seg(r.left(), y, r.width(), segH);

        const bool on = (idxFromBottom < lit);

        const float ht = (float)idxFromBottom / (segments - 1); // 0 bottom .. 1 top
        QColor c = (ht < 0.5f) ? lerp(low, mid, ht * 2.f)
                               : lerp(mid, high, (ht - 0.5f) * 2.f);

        if (!on) c.setAlpha(45);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(seg, 4, 4);
    }

    // Peak marker line
    const int peakIdx = qBound(0, (int)qRound(peakHold * segments), segments);
    if (peakIdx > 0) {
        const int idxFromBottom = peakIdx - 1;
        const int y = r.bottom() - (idxFromBottom + 1) * segH - idxFromBottom * gap;
        p.setPen(QPen(QColor(230, 200, 255, 180), 2));
        p.drawLine(r.left(), y, r.right(), y);
    }
};

drawColumn(rL, displayL_, peakHoldL_);
drawColumn(rR, displayR_, peakHoldR_);
}
