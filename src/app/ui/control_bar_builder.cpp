#include "app/ui/control_bar_builder.hpp"

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>

#include "app/window.hpp" // Dash definition
#include "app/arbiter.hpp"

#include "app/widgets/cpu_temp_widget.hpp"
#include "app/widgets/control_bluetooth_button.hpp"
#include "app/widgets/control_power_button.hpp"

namespace Ui {

QWidget *build_control_bar(Dash *dash)
{
    // dash must exist (we use it to call open_settings_bluetooth())
    if (!dash) return nullptr;

    // IMPORTANT: control_bar() is called from Dash and expects a QWidget*
    auto widget = new QWidget();
    widget->setObjectName("ControlBar");

    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(10);

    // ----- Left side space / padding (keep if you want icons grouped right) -----
    layout->addStretch(1);

    // ----- CPU temp (your extracted widget) -----
    auto cpuTemp = new CpuTempWidget(widget);
    layout->addWidget(cpuTemp);

    // ----- Bluetooth control button (your extracted widget) -----
    auto bt = new ControlBluetoothButton(dash->get_arbiter(), widget, 32);
    layout->addWidget(bt);

    // Click -> open Settings -> Bluetooth tab (Dash helper)
    QObject::connect(bt, &QPushButton::clicked, bt, [dash]{
        dash->open_settings_bluetooth_public();
    });

    // ----- Power button (your extracted widget) -----
    auto powerBtn = new ControlPowerButton(dash->get_arbiter(), widget, 32);
    layout->addWidget(powerBtn);

    return widget;
}

} // namespace Ui
