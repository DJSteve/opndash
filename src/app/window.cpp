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
#include <QTextStream>
#include <QTabWidget>
#include <QSlider>
#include <QBoxLayout>
#include <QVBoxLayout>

#include "app/window.hpp"
#include "app/widgets/pulse_vu_meter.hpp"
#include "app/widgets/nav_neon_indicator.hpp"
#include "app/widgets/nav_neon_resize_filter.hpp"
#include "app/widgets/cpu_temp_widget.hpp"
#include "app/widgets/control_bluetooth_button.hpp"
#include "app/widgets/control_power_button.hpp"
#include "app/ui/control_bar_builder.hpp"
#include "app/ui/status_bar_builder.hpp"
#include "app/ui/transition_manager.hpp"


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
    // Load external theme stylesheet
    {
    	QFile f("/home/steve/dash/theme/theme.qss");
    		if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        QString qss = ts.readAll();
        this->setStyleSheet(qss);
    	}
    }

    this->ensure_transition_overlay();

    this->body.status_bar->addWidget(this->status_bar());

    this->body.frame_widget->setAttribute(Qt::WA_StyledBackground, true);

    this->rail.widget->setAttribute(Qt::WA_StyledBackground, true);

    this->body.control_bar_widget->setFixedHeight(56);

    this->body.control_bar_widget->setAttribute(Qt::WA_StyledBackground, true);

    this->ensure_nav_neon();

    this->build_nav_rail_and_pages();

    this->body.control_bar->addWidget(this->control_bar());


}

void Dash::set_page(Page *page)
{
    if (!page) return;

    auto id = this->arbiter.layout().page_id(page);

    if (auto b = this->rail.group.button(id)) {
        b->setChecked(true);

        if (this->nav_neon) {
            this->nav_neon->moveToButton(b, true); // animate to active button
        }
    }

    // All page switching + overlay transition handled here:
    Ui::transition_to_page(this, page);
}


Arbiter &Dash::get_arbiter()
{
    return this->arbiter;
}

void Dash::open_settings_bluetooth_public()
{
    this->open_settings_bluetooth();
}


void Dash::open_settings_bluetooth()
{
    Page *settingsPage = nullptr;

    for (auto p : this->arbiter.layout().pages()) {
        if (p && p->name() == "Settings") {
            settingsPage = p;
            break;
        }
    }

    if (!settingsPage)
        return;

    this->set_page(settingsPage);

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
}


QWidget *Dash::status_bar() const
{
    return Ui::build_status_bar(const_cast<Dash*>(this));
}


QWidget *Dash::control_bar() const
{
    // Ui builder expects non-const Dash because it connects to helpers like open_settings_bluetooth()
    return Ui::build_control_bar(const_cast<Dash*>(this));
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
