#include "app/ui/status_bar_builder.hpp"

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

#include "app/window.hpp" // Dash
#include "app/arbiter.hpp"

namespace Ui {

QWidget *build_status_bar(Dash *dash)
{
    if (!dash) return nullptr;

    auto widget = new QWidget();
    widget->setObjectName("StatusBar");

    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(10);

    auto title = new QLabel("---", widget);
    title->setObjectName("StatusTitle");

    auto clock = new QLabel("--:--", widget);
    clock->setObjectName("StatusClock");
    clock->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    layout->addWidget(title);
    layout->addStretch(1);
    layout->addWidget(clock);

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
