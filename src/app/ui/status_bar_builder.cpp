#include "app/ui/status_bar_builder.hpp"

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

#include "app/window.hpp" // Dash
#include "app/arbiter.hpp"
#include "app/widgets/now_playing_widget.hpp"

namespace Ui {

QWidget *build_status_bar(Dash *dash)
{
    if (!dash) return nullptr;

    auto widget = new QWidget();
    widget->setObjectName("StatusBar");

    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(10);

    // Now Playing (fills the empty space)
    auto np = new NowPlayingWidget(widget);
    np->setSourceTag("—");
    np->setNowPlaying(""); // shows "No media" via your widget logic
    layout->addWidget(np, 1);

    // Clock (right side)
    auto clock = new QLabel("--:--", widget);
    clock->setObjectName("StatusClock");
    clock->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    layout->addWidget(clock);

    // Wire arbiter -> now playing widget
    auto &arb = dash->get_arbiter();
    QObject::connect(&arb, &Arbiter::now_playing_changed, widget,
        [np](const QString &src, const QString &text, bool active){
            np->setSourceTag(src);
            np->setNowPlaying(text);
            np->setActive(active);
        });

    // update clock once per second
    auto timer = new QTimer(widget);
    timer->setInterval(1000);
    QObject::connect(timer, &QTimer::timeout, widget, [clock]{
        clock->setText(QDateTime::currentDateTime().toString("HH:mm"));
    });
    timer->start();

    // run immediately
    clock->setText(QDateTime::currentDateTime().toString("HH:mm"));

    return widget;
}

} // namespace Ui
