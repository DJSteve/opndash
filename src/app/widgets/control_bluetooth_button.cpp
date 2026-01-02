#include "app/widgets/control_bluetooth_button.hpp"

#include <QTimer>
#include <QToolTip>
#include <QStringList>
#include <QPoint>
#include <QStyle>
#include <QVariant>

#include "app/arbiter.hpp"
#include "app/services/bluetooth.hpp"

ControlBluetoothButton::ControlBluetoothButton(Arbiter &arbiter, QWidget *parent, int iconPx)
    : QPushButton(parent)
    , arbiter_(arbiter)
    , iconPx_(iconPx)
{
    setFlat(true);
    setToolTip("Bluetooth");

    setObjectName("ControlBluetooth");
    setProperty("bt_state", QVariant(QStringLiteral("idle")));
    setProperty("bt_pulse", 0);

    // icon (matches your theme.qss rules)
    arbiter_.forge().iconize("bluetooth_control", this, iconPx_);

    // Pulse timer (used only while scanning)
    pulseTimer_ = new QTimer(this);
    pulseTimer_->setInterval(500);
    pulseTimer_->setSingleShot(false);

    connect(pulseTimer_, &QTimer::timeout, this, [this]{
        const int v = property("bt_pulse").toInt();
        setPulse(v ? 0 : 1);
    });

    // Initial refresh once Bluetooth service finishes init
    connect(&arbiter_.system().bluetooth, &Bluetooth::init, this, [this]{
        refresh();
    });

    // Scanning status
    connect(&arbiter_.system().bluetooth, &Bluetooth::scan_status, this, [this](bool status){
        scanning_ = status;
        refresh();
    });

    // Device connection changes
    connect(&arbiter_.system().bluetooth, &Bluetooth::device_added, this, [this](BluezQt::DevicePtr){
        refresh();
    });
    connect(&arbiter_.system().bluetooth, &Bluetooth::device_changed, this, [this](BluezQt::DevicePtr){
        refresh();
    });
    connect(&arbiter_.system().bluetooth, &Bluetooth::device_removed, this, [this](BluezQt::DevicePtr){
        refresh();
    });

    // Media player changes (also indicates a “connected media device”)
    connect(&arbiter_.system().bluetooth, &Bluetooth::media_player_changed, this, [this](QString, BluezQt::MediaPlayerPtr){
        refresh();
    });

    // ---- Long-press tooltip (connection / scanning info) ----
    connect(this, &QPushButton::pressed, this, [this]{
        QTimer::singleShot(550, this, [this]{
            if (!isDown())
                return;

            const QString tip = buildTooltip();
            const QPoint pos = mapToGlobal(QPoint(width() / 2, 0));
            QToolTip::showText(pos, tip, this);
        });
    });

    connect(this, &QPushButton::released, this, []{
        QToolTip::hideText();
    });

    // Run once now (best effort)
    refresh();
}

void ControlBluetoothButton::setPulse(int v)
{
    setProperty("bt_pulse", v);

    // re-apply QSS property selectors
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ControlBluetoothButton::setBtState(const char *state)
{
    setProperty("bt_state", QVariant(QString::fromLatin1(state)));

    // re-apply QSS property selectors
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ControlBluetoothButton::refresh()
{
    // Connected if ANY known device is connected
    bool connected = false;
    for (auto dev : arbiter_.system().bluetooth.get_devices()) {
        if (dev && dev->isConnected()) {
            connected = true;
            break;
        }
    }

    if (scanning_) {
        setBtState("scanning");
        if (!pulseTimer_->isActive())
            pulseTimer_->start();
    } else if (connected) {
        setBtState("connected");
        if (pulseTimer_->isActive())
            pulseTimer_->stop();
        setPulse(0);
    } else {
        setBtState("idle");
        if (pulseTimer_->isActive())
            pulseTimer_->stop();
        setPulse(0);
    }
}

QString ControlBluetoothButton::buildTooltip() const
{
    QStringList connectedNames;

    for (auto dev : arbiter_.system().bluetooth.get_devices()) {
        if (dev && dev->isConnected())
            connectedNames << dev->name();
    }

    QString tip = "Bluetooth\n";
    tip += scanning_ ? "Scanning: yes\n" : "Scanning: no\n";

    if (connectedNames.isEmpty())
        tip += "Connected: none";
    else
        tip += "Connected: " + connectedNames.join(", ");

    return tip;
}
