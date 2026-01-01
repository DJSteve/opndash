#include "app/widgets/nav_neon_indicator.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QWidget>

NavNeonIndicator::NavNeonIndicator(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    anim = new QPropertyAnimation(this, "yPos", this);
    anim->setDuration(180);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    // start hidden until first button is known
    hide();
}

void NavNeonIndicator::setYPos(int y)
{
    y_pos = y;
    update();
}

void NavNeonIndicator::moveToButton(QWidget *button, bool animate)
{
    if (!button) return;

    // Put the indicator inside the same parent as the buttons (the rail widget)
    if (!isVisible()) show();

    // match button height
    h = button->height();

    // y relative to parent
    const int targetY = button->y();

    if (!animate) {
        setYPos(targetY);
        return;
    }

    anim->stop();
    anim->setStartValue(y_pos);
    anim->setEndValue(targetY);
    anim->start();
}

void NavNeonIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // indicator rect within rail width
    QRect r(margin, y_pos + 4, width() - (margin * 2), h - 8);

    // Base fill
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawRoundedRect(r, 14, 14);

    // “Glow” = layered strokes (cheap but looks great)
    // Outer purple halo
    QPen pen1(glowOuter);
    pen1.setWidth(6);
    p.setBrush(Qt::NoBrush);
    p.setPen(pen1);
    p.drawRoundedRect(r.adjusted(-2, -2, 2, 2), 16, 16);

    // Inner cyan edge
    QPen pen2(glowInner);
    pen2.setWidth(2);
    p.setPen(pen2);
    p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 14, 14);
}
