#include "app/ui/transition_manager.hpp"

#include <QTimer>

#include "app/window.hpp"

namespace Ui {

void transition_to_page(Dash *dash, Page *page)
{
    if (!dash || !page)
        return;

    // If transitions not set up, just switch immediately
    if (!dash->transition_overlay || !dash->transition_fx || !dash->transition_anim) {
        dash->body.frame->setCurrentWidget(page->container());
        return;
    }

    // If already transitioning, queue the latest request
    if (dash->transitioning) {
        dash->pending_page = page;
        return;
    }

    dash->transitioning = true;
    dash->pending_page = nullptr;

    dash->transition_overlay->setGeometry(dash->body.frame_widget->rect());
    dash->transition_overlay->raise();
    dash->transition_overlay->show();

    // Fade in overlay quickly
    dash->transition_anim->stop();
    dash->transition_anim->setStartValue(0.0);
    dash->transition_anim->setEndValue(1.0);
    dash->transition_anim->start();

    // After fade-in, swap page, then fade out
    QTimer::singleShot(70, dash, [dash, page]{
        dash->body.frame->setCurrentWidget(page->container());

        dash->transition_anim->stop();
        dash->transition_anim->setStartValue(1.0);
        dash->transition_anim->setEndValue(0.0);
        dash->transition_anim->start();

        QTimer::singleShot(140, dash, [dash]{
            dash->transition_overlay->hide();
            dash->transitioning = false;

            // If another request came in during the animation, run it now
            if (dash->pending_page) {
                Page *next = dash->pending_page;
                dash->pending_page = nullptr;
                transition_to_page(dash, next);
            }
        });
    });
}

} // namespace Ui
