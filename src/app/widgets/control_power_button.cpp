#include "app/widgets/control_power_button.hpp"

#include <QHBoxLayout>
#include <QPushButton>

#include "app/arbiter.hpp"
#include "app/session.hpp"
#include "app/widgets/dialog.hpp"   // Dialog class in your project
#include "app/pages/settings.hpp" // for arbiter.settings().sync() (path matches your tree)

ControlPowerButton::ControlPowerButton(Arbiter &arbiter, QWidget *parent, int iconPx)
    : QPushButton(parent)
    , arbiter_(arbiter)
{
    setFlat(true);

    // Use your existing power icon resource
    arbiter_.forge().iconize("power_settings_new", this, iconPx);

    connect(this, &QPushButton::clicked, this, [this]{
        ensureDialog();
        if (dialog_) dialog_->show();
    });
}

void ControlPowerButton::ensureDialog()
{
    if (dialog_)
        return;

    dialog_ = new Dialog(arbiter_, true, arbiter_.window());
    dialog_->set_title("Power Off");
    dialog_->set_body(buildPowerControl());
}

QWidget *ControlPowerButton::buildPowerControl() const
{
    auto widget = new QWidget();
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Restart button
    auto restart = new QPushButton();
    restart->setFlat(true);
    arbiter_.forge().iconize("refresh", restart, 36);

    QObject::connect(restart, &QPushButton::clicked, [this]{
        arbiter_.settings().sync();
        ::sync();
        ::system(Session::System::REBOOT_CMD);
    });

    layout->addWidget(restart);

    // Power off button
    auto power_off = new QPushButton();
    power_off->setFlat(true);
    arbiter_.forge().iconize("power_settings_new", power_off, 36);

    QObject::connect(power_off, &QPushButton::clicked, [this]{
        arbiter_.settings().sync();
        ::sync();
        ::system(Session::System::SHUTDOWN_CMD);
    });

    layout->addWidget(power_off);

    return widget;
}
