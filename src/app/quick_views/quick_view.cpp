#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Qt>

#include "app/arbiter.hpp"

#include "app/quick_views/quick_view.hpp"

QuickView::QuickView(Arbiter &arbiter, QString name, QWidget *widget)
    : arbiter(arbiter)
    , name_(name)
    , widget_(widget)
{
}

NullQuickView::NullQuickView(Arbiter &arbiter)
    : QFrame()
    , QuickView(arbiter, "none", this)
{
}

void NullQuickView::init()
{
}

VolumeQuickView::VolumeQuickView(Arbiter &arbiter)
    : QFrame()
    , QuickView(arbiter, "volume", this)
{
}

void VolumeQuickView::init()
{
//    auto layout = new QHBoxLayout(this);
//    layout->setContentsMargins(0, 0, 0, 0);

//    layout->addWidget(this->arbiter.forge().volume_slider());

    auto w = this->widget();

    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto vol = this->arbiter.forge().volume_slider(Qt::Vertical, true);
    w->setMinimumHeight(240);
    w->setMinimumWidth(64);

    layout->addWidget(vol, 1, Qt::AlignHCenter);
}

BrightnessQuickView::BrightnessQuickView(Arbiter &arbiter)
    : QFrame()
    , QuickView(arbiter, "brightness", this)
{
}

void BrightnessQuickView::init()
{
//    auto layout = new QHBoxLayout(this);
//    layout->setContentsMargins(0, 0, 0, 0);

//    layout->addWidget(this->arbiter.forge().brightness_slider());

    auto w = this->widget();

    auto layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto bri = this->arbiter.forge().brightness_slider(Qt::Vertical, true);
    w->setMinimumHeight(240);
    w->setMinimumWidth(64);

    layout->addWidget(bri, 1, Qt::AlignHCenter);
}
