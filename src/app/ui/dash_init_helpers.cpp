#include "app/window.hpp"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

#include "app/widgets/nav_neon_indicator.hpp"
#include "app/widgets/nav_neon_resize_filter.hpp"

void Dash::ensure_transition_overlay()
{
    // ---- Safe page transition overlay (does not touch page widgets / OpenGL) ----
    if (this->transition_overlay)
        return;

    this->transition_overlay = new QWidget(this->body.frame_widget);
    this->transition_overlay->setObjectName("TransitionOverlay");
    this->transition_overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    this->transition_overlay->setGeometry(this->body.frame_widget->rect());
    this->transition_overlay->raise();
    this->transition_overlay->hide();

    this->transition_fx = new QGraphicsOpacityEffect(this->transition_overlay);
    this->transition_overlay->setGraphicsEffect(this->transition_fx);
    this->transition_fx->setOpacity(0.0);

    this->transition_anim = new QPropertyAnimation(this->transition_fx, "opacity", this);
    this->transition_anim->setEasingCurve(QEasingCurve::OutExpo);
    this->transition_anim->setDuration(160);
}

void Dash::ensure_nav_neon()
{
    // Neon indicator (behind the buttons)
    if (this->nav_neon)
        return;

    this->nav_neon = new NavNeonIndicator(this->rail.widget);
    this->nav_neon->setGeometry(this->rail.widget->rect());
    this->nav_neon->lower(); // stay behind buttons
    this->nav_neon->show();

    // keep it full-height if the rail resizes
    this->rail.widget->installEventFilter(new NavNeonResizeFilter(this->nav_neon, this->rail.widget));
}
