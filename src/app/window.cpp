#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include "app/widgets/rail_volume.hpp"

#include "app/quick_views/quick_view.hpp"
#include "app/utilities/icon_engine.hpp"
#include "app/widgets/dialog.hpp"

//Bluetooth Required Includes
#include <QTabWidget>
#include <QTimer>
#include <QToolTip>
//Include needed for PI5 Cpu temp
#include <QLabel>
#include <QFile>

#include <QSlider>
#include <QBoxLayout>
#include <QVBoxLayout>

#include "app/window.hpp"
#include "app/widgets/pulse_vu_meter.hpp"

Dash::NavRail::NavRail()
    : group()
    , timer()
    , widget(new QWidget())
    , layout(new QVBoxLayout(widget))
{
    this->widget->setObjectName("NavRail");
    this->widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    this->widget->setMinimumWidth(64);  // tweak later, but ensures it stays visible

    this->layout->setContentsMargins(0, 0, 0, 0);
    this->layout->setSpacing(0);
}

Dash::Body::Body()
    : layout(new QVBoxLayout())
    , status_bar(new QVBoxLayout())
    , frame(new QStackedLayout())
    , control_bar(new QVBoxLayout())
{
    this->layout->setContentsMargins(0, 0, 0, 0);
    this->layout->setSpacing(0);

    this->status_bar->setContentsMargins(0, 0, 0, 0);
    this->layout->addLayout(this->status_bar);

// Frame widget that will actually paint the background
this->frame_widget = new QWidget();
this->frame_widget->setObjectName("DashFrame");

// Put the stacked layout onto that widget
this->frame = new QStackedLayout(this->frame_widget);
this->frame->setContentsMargins(0, 0, 0, 0);

// Add the widget (not the layout) to the main layout
this->layout->addWidget(this->frame_widget, 1);

//    this->frame->setContentsMargins(0, 0, 0, 0);
//    this->layout->addLayout(this->frame, 1);

    auto msg_ref = new QWidget();
    msg_ref->setObjectName("MsgRef");
    this->layout->addWidget(msg_ref);

this->control_bar_widget = new QWidget();
this->control_bar_widget->setObjectName("ControlBar");

this->control_bar = new QVBoxLayout(this->control_bar_widget);
this->control_bar->setContentsMargins(0, 0, 0, 0);

this->layout->addWidget(this->control_bar_widget);

//    this->control_bar->setContentsMargins(0, 0, 0, 0);
//    this->layout->addLayout(this->control_bar);
}

Dash::Dash(Arbiter &arbiter)
    : QWidget()
    , arbiter(arbiter)
    , rail()
    , body()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(this->rail.widget);
    layout->addLayout(this->body.layout);

    connect(&this->rail.group, QOverload<int>::of(&QButtonGroup::buttonPressed), [this](int id){
        this->arbiter.set_curr_page(id);
        this->rail.timer.start();
    });
    connect(&this->rail.group, QOverload<int>::of(&QButtonGroup::buttonReleased), [this](int id){
        if (this->rail.timer.hasExpired(1000))
            this->arbiter.set_fullscreen(true);
    });
    connect(&this->arbiter, &Arbiter::curr_page_changed, [this](Page *page){
        this->set_page(page);
    });
    connect(&this->arbiter, &Arbiter::page_changed, [this](Page *page, bool enabled){
        int id = this->arbiter.layout().page_id(page);
        this->rail.group.button(id)->setVisible(enabled);

        if ((this->arbiter.layout().curr_page == page) && !enabled)
            this->arbiter.set_curr_page(this->arbiter.layout().next_enabled_page(page));
    });
}

void Dash::init()
{
    // ---- Safe page transition overlay (does not touch page widgets / OpenGL) ----
    if (!this->transition_overlay) {
        this->transition_overlay = new QWidget(this->body.frame_widget);
        this->transition_overlay->setObjectName("TransitionOverlay");
        this->transition_overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        this->transition_overlay->setGeometry(this->body.frame_widget->rect());
        this->transition_overlay->raise();
        this->transition_overlay->hide();

        // Dark purple cover to mask page switch
	this->transition_overlay->setStyleSheet(R"(
	    background: qradialgradient(
	        cx:0.5, cy:0.5, radius:0.9,
	        stop:0 rgba(180, 120, 255, 220),
	        stop:0.4 rgba(90, 40, 200, 200),
	        stop:0.75 rgba(20, 10, 60, 230),
	        stop:1 rgba(10, 8, 20, 255)
	    );
	)");

        this->transition_fx = new QGraphicsOpacityEffect(this->transition_overlay);
        this->transition_overlay->setGraphicsEffect(this->transition_fx);
        this->transition_fx->setOpacity(0.0);

        this->transition_anim = new QPropertyAnimation(this->transition_fx, "opacity", this);
        this->transition_anim->setEasingCurve(QEasingCurve::OutExpo);
        this->transition_anim->setDuration(160);
    }



    this->body.status_bar->addWidget(this->status_bar());

    this->body.frame_widget->setAttribute(Qt::WA_StyledBackground, true);
    this->body.frame_widget->setStyleSheet(R"(
    #DashFrame {
        background: qlineargradient(
            x1:0, y1:0,
            x2:0, y2:1,
            stop:0 #070814,
            stop:0.45 #1A1A5E,
            stop:1 #4B118A
        );
    }
    )");

    this->rail.widget->setAttribute(Qt::WA_StyledBackground, true);
    this->rail.widget->setStyleSheet(R"(
    #NavRail {
        background: qlineargradient(
            x1:0, y1:0,
            x2:0, y2:1,
            stop:0 #0D0E28,
            stop:1 #5A189A
        );
        border-right: 1px solid rgba(0, 0, 0, 180);          /* shadow edge */
        border-left: 1px solid rgba(220, 170, 255, 100);      /* highlight edge */
    }

    /* All nav buttons */
    #NavRail QPushButton {
        background: transparent;
        border: 0px;
        margin: 6px 4px;
        padding: 6px;
        border-radius: 10px;
    }

    #NavRail QPushButton:checked {
        background: qradialgradient(
            cx:0.5, cy:0.5, radius:0.9,
            stop:0 rgba(190, 130, 255, 200),
            stop:0.5 rgba(120, 60, 220, 120),
            stop:1 rgba(0, 0, 0, 0)
        );
    }

    /* Optional: press feedback */
    #NavRail QPushButton:pressed {
        background: rgba(200, 140, 255, 80);
    }

    )");

    this->body.control_bar_widget->setFixedHeight(56);

    this->body.control_bar_widget->setAttribute(Qt::WA_StyledBackground, true);
    this->body.control_bar_widget->setStyleSheet(R"(
    #ControlBar {
        background: qlineargradient(
            x1:0, y1:0,
            x2:0, y2:1,
            stop:0 #4B118A,
            stop:1 #2D0F55
        );
        border-top: 1px solid rgba(200, 140, 255, 60);
    }
    )");

    for (auto page : this->arbiter.layout().pages()) {
        auto button = page->button();
        button->setCheckable(true);
        button->setFlat(true);

	button->setMinimumSize(56, 56);
	button->setMaximumSize(56, 56);
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QIcon icon(new StylizedIconEngine(this->arbiter, QString(":/icons/%1.svg").arg(page->icon_name()), true));
        this->arbiter.forge().iconize(icon, button, 32);

        this->rail.group.addButton(button, this->arbiter.layout().page_id(page));
        this->rail.layout->addWidget(button);
        this->body.frame->addWidget(page->container());

        page->init();
        button->setVisible(page->enabled());
    }
    // Push the rail buttons up so the quick view sits at the bottom
    this->rail.layout->addStretch(1);

// ---- VU meter (below page buttons) ----
auto vu = new PulseVUMeter(this->rail.widget);
vu->setFixedWidth(64);
vu->setFixedHeight(540);
this->rail.layout->addWidget(vu, 0, Qt::AlignHCenter);
this->rail.layout->addSpacing(10);

// (optional) if you need the volume widget pinned to bottom:
this->rail.layout->addStretch(1);

auto railVol = new RailVolume(this->arbiter, this);
this->rail.layout->addStretch(1);         // push it toward the bottom
this->rail.layout->addWidget(railVol);

    this->set_page(this->arbiter.layout().curr_page);

    this->body.control_bar->addWidget(this->control_bar());
}

void Dash::set_page(Page *page)
{
    auto id = this->arbiter.layout().page_id(page);
    if (auto b = this->rail.group.button(id))
        b->setChecked(true);

    QWidget *target = page->container();
    QWidget *current = this->body.frame->currentWidget();
    if (current == target)
        return;

    // If transition system isn't ready, just switch
    if (!this->transition_overlay || !this->transition_fx || !this->transition_anim) {
        this->body.frame->setCurrentWidget(target);
        return;
    }

    // Queue if we are mid-transition
    if (this->transitioning) {
        this->pending_page = page;
        return;
    }

    this->transitioning = true;
    this->pending_page = nullptr;

    // Ensure overlay covers the frame (handles fullscreen/resizes)
    this->transition_overlay->setGeometry(this->body.frame_widget->rect());
    this->transition_overlay->show();
    this->transition_overlay->raise();

    // Stop any previous run cleanly
    this->transition_anim->stop();
    QObject::disconnect(this->transition_anim, nullptr, nullptr, nullptr);

    // Phase 1: fade overlay IN to cover
    this->transition_anim->setStartValue(0.0);
    this->transition_anim->setEndValue(1.0);

    QObject::connect(this->transition_anim, &QPropertyAnimation::finished, this, [this, page]() {
        // Switch while fully covered (no ghosting / no OpenGL effect crashes)
        this->body.frame->setCurrentWidget(page->container());

        // Phase 2: fade overlay OUT to reveal
        this->transition_anim->stop();
        QObject::disconnect(this->transition_anim, nullptr, nullptr, nullptr);
        this->transition_anim->setStartValue(1.0);
        this->transition_anim->setEndValue(0.0);

        QObject::connect(this->transition_anim, &QPropertyAnimation::finished, this, [this]() {
            this->transition_overlay->hide();
            this->transitioning = false;

            if (this->pending_page) {
                Page *next = this->pending_page;
                this->pending_page = nullptr;
                this->set_page(next);
            }
        });

        this->transition_anim->start();
    });

    this->transition_anim->start();
}



QWidget *Dash::status_bar() const
{
    auto widget = new QWidget();
    widget->setObjectName("StatusBar");

widget->setAttribute(Qt::WA_StyledBackground, true);
widget->setStyleSheet(R"(
    #StatusBar {
        background: qlineargradient(
            x1:0, y1:0,
            x2:0, y2:1,
            stop:0 #120A2A,
            stop:1 #2A0A5E
        );
        border-top: 1px solid rgba(180, 120, 255, 60);
    }
)");
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto clock = new QLabel();
    clock->setFont(this->arbiter.forge().font(10, true));
    clock->setAlignment(Qt::AlignCenter);
    layout->addWidget(clock);

    connect(&this->arbiter.system().clock, &Clock::ticked, [clock](QTime time){
        clock->setText(QLocale().toString(time, QLocale::ShortFormat));
    });

    widget->setVisible(this->arbiter.layout().status_bar);
    connect(&this->arbiter, &Arbiter::status_bar_changed, [widget](bool enabled){
        widget->setVisible(enabled);
    });

    return widget;
}

QWidget *Dash::control_bar() const
{
    auto widget = new QWidget();
    widget->setObjectName("ControlBar");
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

//disabling quick views here so i can move them to the sidebar - better layout for a portrait screen
//    auto quick_views = new QStackedLayout();
//   quick_views->setContentsMargins(0, 0, 0, 0);
//    layout->addLayout(quick_views);
//    for (auto quick_view : this->arbiter.layout().control_bar.quick_views()) {
//        quick_views->addWidget(quick_view->widget());
//        quick_view->init();
//    }
//    quick_views->setCurrentWidget(this->arbiter.layout().control_bar.curr_quick_view->widget());
//    connect(&this->arbiter, &Arbiter::curr_quick_view_changed, [quick_views](QuickView *quick_view){
//        quick_views->setCurrentWidget(quick_view->widget());
//    });

auto cpuTemp = new QLabel("--.-°C");
cpuTemp->setObjectName("CpuTemp");
cpuTemp->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
cpuTemp->setMinimumWidth(70); // enough for "59.2°C"
cpuTemp->setStyleSheet(R"(
    QLabel#CpuTemp {
        color: rgba(230, 200, 255, 210);
        font-size: 12px;
        padding-right: 8px;
    }
)");
layout->addWidget(cpuTemp, 0);

auto read_cpu_temp_c = []() -> double {
    QFile f("/sys/class/thermal/thermal_zone0/temp");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1.0;

    const QByteArray raw = f.readAll().trimmed();
    bool ok = false;
    const long v = raw.toLong(&ok);
    if (!ok) return -1.0;

    // Most kernels report millidegrees C
    if (v > 1000) return v / 1000.0;
    return static_cast<double>(v);
};

auto tempTimer = new QTimer(cpuTemp);
tempTimer->setInterval(2000); // 1s update; change to 2000 if you prefer

    connect(tempTimer, &QTimer::timeout, cpuTemp, [cpuTemp, read_cpu_temp_c]{
    const double t = read_cpu_temp_c();
    if (t < 0.0) {
        cpuTemp->setText("--.-°C");
        cpuTemp->setStyleSheet(R"(
            QLabel#CpuTemp { color: rgba(180, 180, 180, 180); font-size: 12px; padding-right: 8px; }
        )");
        return;
    }

    cpuTemp->setText(QString::asprintf("%.1f°C", t));

    // Colour bands (Pi 5: warm starts ~70C, hot ~80C)
    if (t < 60.0) {
        // Cool: bright cyan-ish (fits your purple/blue theme)
        cpuTemp->setStyleSheet(R"(
            QLabel#CpuTemp { color: rgba(80, 230, 255, 230); font-size: 12px; padding-right: 8px; }
        )");
    } else if (t < 70.0) {
        // Normal: lilac
        cpuTemp->setStyleSheet(R"(
            QLabel#CpuTemp { color: rgba(200, 140, 255, 230); font-size: 12px; padding-right: 8px; }
        )");
    } else if (t < 80.0) {
        // Warm: amber
        cpuTemp->setStyleSheet(R"(
            QLabel#CpuTemp { color: rgba(255, 190, 90, 240); font-size: 12px; padding-right: 8px; }
        )");
    } else {
        // Hot: red
        cpuTemp->setStyleSheet(R"(
            QLabel#CpuTemp { color: rgba(255, 80, 80, 240); font-size: 12px; padding-right: 8px; font-weight: 600; }
        )");
    }
});
tempTimer->start();

// Run once immediately so it shows on boot without waiting 1s
QTimer::singleShot(0, cpuTemp, [cpuTemp, read_cpu_temp_c]{
    const double t = read_cpu_temp_c();
    if (t < 0.0) {
        cpuTemp->setText("--.-°C");
        return;
    }
    cpuTemp->setText(QString::asprintf("%.1f°C", t));
});


    layout->addStretch();

    auto bluetooth = new QPushButton();
    bluetooth->setFlat(true);
    bluetooth->setToolTip("Bluetooth");
    this->arbiter.forge().iconize("bluetooth_control", bluetooth, 32);
    layout->addWidget(bluetooth);
    bluetooth->setObjectName("ControlBluetooth");
    bluetooth->setProperty("bt_state", "idle");

    bluetooth->setProperty("bt_pulse", 0);

    auto pulseTimer = new QTimer(bluetooth);
    pulseTimer->setInterval(500);  // 400–700ms feels good
    pulseTimer->setSingleShot(false);

    connect(pulseTimer, &QTimer::timeout, bluetooth, [bluetooth]{
    const int v = bluetooth->property("bt_pulse").toInt();
    bluetooth->setProperty("bt_pulse", v ? 0 : 1);

    // Re-apply stylesheet so the property change takes effect
    bluetooth->style()->unpolish(bluetooth);
    bluetooth->style()->polish(bluetooth);
    bluetooth->update();
    });

    // Base style + state styles (applies only to this button)
    bluetooth->setStyleSheet(R"(
    QPushButton#ControlBluetooth {
        border: 1px solid rgba(200, 140, 255, 40);
        border-radius: 12px;
        padding: 6px;
        background: rgba(0, 0, 0, 0);
    }

    /* Connected = stronger purple glow */
    QPushButton#ControlBluetooth[bt_state="connected"] {
        border: 1px solid rgba(220, 170, 255, 130);
        background: qradialgradient(
            cx:0.5, cy:0.5, radius:0.9,
            stop:0 rgba(190, 130, 255, 140),
            stop:0.6 rgba(80, 30, 160, 70),
            stop:1 rgba(0, 0, 0, 0)
        );
    }

    /* Scanning = brighter “active” glow */
    QPushButton#ControlBluetooth[bt_state="scanning"] {
    	border: 2px solid rgba(255, 210, 255, 200);
    	background: qlineargradient(
        x1:0, y1:0,
        x2:1, y2:0,
        stop:0 rgba(210, 160, 255, 40),
        stop:0.5 rgba(210, 160, 255, 170),
        stop:1 rgba(210, 160, 255, 40)
    );
    }
    /* Scanning pulse "ON" phase */
    QPushButton#ControlBluetooth[bt_state="scanning"][bt_pulse="1"] {
    border: 2px solid rgba(255, 230, 255, 235);
    background: qlineargradient(
        x1:0, y1:0,
        x2:1, y2:0,
        stop:0 rgba(230, 190, 255, 70),
        stop:0.5 rgba(230, 190, 255, 210),
        stop:1 rgba(230, 190, 255, 70)
    );
    }
    )");
    auto scanning = std::make_shared<bool>(false);

    auto apply_bt_state = [this, bluetooth, scanning, pulseTimer]() {
    // Connected if ANY known device is connected
    bool connected = false;
    for (auto dev : this->arbiter.system().bluetooth.get_devices()) {
        if (dev && dev->isConnected()) { connected = true; break; }
    }

    if (*scanning) {
        this->arbiter.forge().iconize("bluetooth_control", bluetooth, 32);
    } else if (connected) {
        this->arbiter.forge().iconize("bluetooth_control_connected", bluetooth, 32);
    } else {
        this->arbiter.forge().iconize("bluetooth_control", bluetooth, 32);
    }

    const char *state =
        (*scanning) ? "scanning" :
        (connected) ? "connected" :
                      "idle";

    bluetooth->setProperty("bt_state", state);

    // Force Qt to re-apply the stylesheet based on the new property
    bluetooth->style()->unpolish(bluetooth);
    bluetooth->style()->polish(bluetooth);
    bluetooth->update();

    if (*scanning) {
    if (!pulseTimer->isActive())
        pulseTimer->start();
    } else {
    if (pulseTimer->isActive())
        pulseTimer->stop();

    // reset pulse to off so it doesn't get stuck bright
    bluetooth->setProperty("bt_pulse", 0);
    }
    };

// Initial refresh once Bluetooth service finishes init
connect(&this->arbiter.system().bluetooth, &Bluetooth::init, bluetooth, [apply_bt_state]() {
    apply_bt_state();
});

// Scanning status
connect(&this->arbiter.system().bluetooth, &Bluetooth::scan_status, bluetooth, [scanning, apply_bt_state](bool status) {
    *scanning = status;
    apply_bt_state();
});

// Device connection changes
connect(&this->arbiter.system().bluetooth, &Bluetooth::device_added, bluetooth, [apply_bt_state](BluezQt::DevicePtr) {
    apply_bt_state();
});
connect(&this->arbiter.system().bluetooth, &Bluetooth::device_changed, bluetooth, [apply_bt_state](BluezQt::DevicePtr) {
    apply_bt_state();
});
connect(&this->arbiter.system().bluetooth, &Bluetooth::device_removed, bluetooth, [apply_bt_state](BluezQt::DevicePtr) {
    apply_bt_state();
});

// Media player device changes (also indicates a “connected media device”)
connect(&this->arbiter.system().bluetooth, &Bluetooth::media_player_changed, bluetooth, [apply_bt_state](QString, BluezQt::MediaPlayerPtr) {
    apply_bt_state();
});

// Run once now (best effort)
apply_bt_state();

// ---- Long-press tooltip (connection / scanning info) ----
auto build_bt_tooltip = [this, scanning]() -> QString {
    QStringList connectedNames;

    for (auto dev : this->arbiter.system().bluetooth.get_devices()) {
        if (dev && dev->isConnected())
            connectedNames << dev->name();
    }

    QString tip = "Bluetooth\n";

    if (*scanning)
        tip += "Scanning: yes\n";
    else
        tip += "Scanning: no\n";

    if (connectedNames.isEmpty()) {
        tip += "Connected: none";
    } else {
        tip += "Connected: " + connectedNames.join(", ");
    }

    return tip;
};

// Press-and-hold to show tooltip (touch-friendly)
connect(bluetooth, &QPushButton::pressed, bluetooth, [bluetooth, build_bt_tooltip]{
    QTimer::singleShot(550, bluetooth, [bluetooth, build_bt_tooltip]{
        if (!bluetooth->isDown())
            return;

        const QString tip = build_bt_tooltip();
        const QPoint pos = bluetooth->mapToGlobal(QPoint(bluetooth->width() / 2, 0));
        QToolTip::showText(pos, tip, bluetooth);
    });
});

// Hide tooltip when released
connect(bluetooth, &QPushButton::released, bluetooth, []{
    QToolTip::hideText();
});

connect(bluetooth, &QPushButton::clicked, [this]{
    Page *settingsPage = nullptr;

    for (auto p : this->arbiter.layout().pages()) {
        if (p && p->name() == "Settings") {
            settingsPage = p;
            break;
        }
    }
    if (!settingsPage) return;

    auto dash = const_cast<Dash*>(this);
    dash->set_page(settingsPage);
QTimer::singleShot(50, dash, [settingsPage]{
    // Page::container() is a PageContainer (QFrame) wrapping the real page widget
    auto cont = settingsPage->container();
    if (!cont || !cont->layout() || cont->layout()->count() < 1)
        return;

    QWidget *inner = cont->layout()->itemAt(0)->widget();
    auto tabs = qobject_cast<QTabWidget*>(inner);
    if (!tabs)
        return;

    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i).contains("Bluetooth", Qt::CaseInsensitive)) {
            tabs->setCurrentIndex(i);
            break;
        }
    }
});

});


    auto dialog = new Dialog(this->arbiter, true, this->arbiter.window());
    dialog->set_title("Power Off");
    dialog->set_body(this->power_control());
    auto shutdown = new QPushButton();
    shutdown->setFlat(true);
    this->arbiter.forge().iconize("power_settings_new", shutdown, 32);
    layout->addWidget(shutdown);
    connect(shutdown, &QPushButton::clicked, [dialog]{ dialog->open(); });

    widget->setVisible(this->arbiter.layout().control_bar.enabled);
    connect(&this->arbiter, &Arbiter::control_bar_changed, [widget](bool enabled){
        widget->setVisible(enabled);
    });

    return widget;
}

QWidget *Dash::power_control() const
{
    auto widget = new QWidget();
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto restart = new QPushButton();
    restart->setFlat(true);
    this->arbiter.forge().iconize("refresh", restart, 36);
    connect(restart, &QPushButton::clicked, [this]{
        this->arbiter.settings().sync();
        sync();
        system(Session::System::REBOOT_CMD);
    });
    layout->addWidget(restart);

    auto power_off = new QPushButton();
    power_off->setFlat(true);
    this->arbiter.forge().iconize("power_settings_new", power_off, 36);
    connect(power_off, &QPushButton::clicked, [this]{
        this->arbiter.settings().sync();
        sync();
        system(Session::System::SHUTDOWN_CMD);
    });
    layout->addWidget(power_off);

    return widget;
}

MainWindow::MainWindow(QRect geometry)
    : QMainWindow()
    , arbiter(this->init(geometry))
    , stack(new QStackedWidget())
{
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    auto frame = new QFrame();
    auto layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(this->stack);
    layout->addWidget(this->arbiter.layout().fullscreen.toggler(1)->widget());

    this->setCentralWidget(frame);

    auto dash = new Dash(this->arbiter);
    this->stack->addWidget(dash);
    dash->init();
//    dash->setStyleSheet("background-color: red;");
    this->arbiter.system().brightness.set();

    if (this->arbiter.layout().fullscreen.on_start)
        this->arbiter.set_fullscreen(true);
}

MainWindow *MainWindow::init(QRect geometry)
{
    // force to either screen or custom size
    this->setFixedSize(geometry.size());
    this->move(geometry.topLeft());

    return this;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    this->arbiter.update();
}

void MainWindow::set_fullscreen(Page *page)
{
    auto widget = page->container()->take();
    this->stack->addWidget(widget);
    this->stack->setCurrentWidget(widget);
}
