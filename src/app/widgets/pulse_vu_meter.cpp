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

    // Outer housing
    const QRectF housing = rect().adjusted(6, 6, -6, -6);

    // "Glass" background (cheap to draw, big visual gain)
    p.setPen(QPen(QColor(255, 255, 255, 25), 1));
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(housing, 14, 14);

    // Leave room INSIDE the widget for the L/R labels at the bottom
    const int labelH = 16;
    const QRect outer = housing.toRect().adjusted(8, 8, -8, -(10 + labelH));

    // Two columns with a small gap + divider line
    const int gapCols = 10;
    const int colW = (outer.width() - gapCols) / 2;

    QRect rL(outer.left(), outer.top(), colW, outer.height());
    QRect rR(outer.left() + colW + gapCols, outer.top(), colW, outer.height());

    // Divider glow (subtle)
    const int divX = outer.left() + colW + (gapCols / 2);
    p.setPen(QPen(QColor(160, 80, 255, 70), 2));
    p.drawLine(divX, outer.top(), divX, outer.bottom());

    // LED segments
    const int segments = 18;   // a bit denser = more "pro"
    const int gap = 3;
    const int segH = (outer.height() - gap * (segments - 1)) / segments;

    // theme gradient: low cyan -> mid purple -> high red
    const QColor low( 60, 230, 255, 220);
    const QColor mid(160,  80, 255, 220);
    const QColor high(255,  80,  80, 230);

    auto lerpC = [](const QColor &a, const QColor &b, float t){
        t = qBound(0.f, t, 1.f);
        return QColor(
            a.red()   + (b.red()   - a.red())   * t,
            a.green() + (b.green() - a.green()) * t,
            a.blue()  + (b.blue()  - a.blue())  * t,
            a.alpha() + (b.alpha() - a.alpha()) * t
        );
    };

    auto segColorFor = [&](int idxFromBottom){
        const float ht = (float)idxFromBottom / (segments - 1); // 0 bottom .. 1 top
        return (ht < 0.5f) ? lerpC(low, mid, ht * 2.f)
                           : lerpC(mid, high, (ht - 0.5f) * 2.f);
    };

    auto drawColumn = [&](const QRect &r, float v, float peakHold){
        v = qBound(0.f, v, 1.f);
        const int lit = qRound(v * segments);

        // LEDs
        for (int i = 0; i < segments; ++i) {
            const int idxFromBottom = i;
            const int y = r.bottom() - (idxFromBottom + 1) * segH - idxFromBottom * gap;
            QRect seg(r.left(), y, r.width(), segH);

            QColor c = segColorFor(idxFromBottom);

            const bool on = (idxFromBottom < lit);
            if (!on) c.setAlpha(40); // darker "off" LEDs

            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(seg, 4, 4);
        }

        // Peak DOT (more like mixer panels than a full line)
        const int peakIdx = qBound(0, (int)qRound(peakHold * segments), segments);
        if (peakIdx > 0) {
            const int idxFromBottom = peakIdx - 1;
            const int y = r.bottom() - (idxFromBottom + 1) * segH - idxFromBottom * gap;
            const QColor pc = segColorFor(idxFromBottom);

            p.setPen(Qt::NoPen);
            p.setBrush(QColor(pc.red(), pc.green(), pc.blue(), 240));

            const int dot = qMax(6, qMin(r.width() / 2, 10));
            QRectF d(r.center().x() - dot/2.0, y + segH/2.0 - dot/2.0, dot, dot);
            p.drawEllipse(d);
        }
    };

    drawColumn(rL, displayL_, peakHoldL_);
    drawColumn(rR, displayR_, peakHoldR_);

    // L / R tiny labels at the bottom (cheap, improves readability)
    p.setPen(QColor(220, 200, 255, 140));
    QFont f = p.font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    f.setBold(true);
    p.setFont(f);

    p.drawText(QRect(rL.left(), rL.bottom() + 2, rL.width(), 14), Qt::AlignHCenter | Qt::AlignTop, "L");
    p.drawText(QRect(rR.left(), rR.bottom() + 2, rR.width(), 14), Qt::AlignHCenter | Qt::AlignTop, "R");
}
