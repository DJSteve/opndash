#include "app/ui/status_bar_builder.hpp"

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

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

    // In your current build the status bar doesn't need much.
    // Keep it minimal but structured so you can add to it later.
    auto title = new QLabel("OpenDash", widget);
    title->setObjectName("StatusTitle");

    layout->addWidget(title);
    layout->addStretch(1);

    return widget;
}

} // namespace Ui
