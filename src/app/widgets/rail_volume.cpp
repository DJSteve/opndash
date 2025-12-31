#include "app/widgets/rail_volume.hpp"

#include "app/arbiter.hpp"
#include "app/session.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QtMath>

static int percent_to_steps32(int percent)
{
    // clamp 0..100
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    // map to 1..32 (0% -> 1, 100% -> 32)
    // using round so midpoints feel natural
    const double f = (percent / 100.0) * 31.0;   // 0..31
    int step = 1 + (int)qRound(f);              // 1..32
    if (step < 1) step = 1;
    if (step > 32) step = 32;
    return step;
}

RailVolume::RailVolume(Arbiter &arbiter, QWidget *parent)
    : QWidget(parent)
    , arbiter_(arbiter)
    , label_(new QLabel(this))
    , up_(new QPushButton(this))
    , down_(new QPushButton(this))
{
    this->setObjectName("RailVolume");

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // Buttons
    up_->setFlat(true);
    down_->setFlat(true);
    up_->setFocusPolicy(Qt::NoFocus);
    down_->setFocusPolicy(Qt::NoFocus);

    // Use your existing icons (resource names must match your .qrc)
    // If your icons are named differently, swap these strings.
    QIcon upIcon(QString(":/icons/volume_up.svg"));
    QIcon downIcon(QString(":/icons/volume_down.svg"));
    up_->setIcon(upIcon);
    down_->setIcon(downIcon);

    // Keep them consistent with your rail icon sizing
    up_->setIconSize(QSize(28, 28));
    down_->setIconSize(QSize(28, 28));

    // Label
    label_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label_->setText("1"); // temporary until signal arrives
    label_->setMinimumWidth(40);

    // Build vertical stack: up, value, down
    layout->addWidget(up_, 0, Qt::AlignHCenter);
    layout->addWidget(label_, 0, Qt::AlignHCenter);
    layout->addWidget(down_, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    // Click handlers: +/- 1 step in 32-step space
    connect(up_, &QPushButton::clicked, [this](){
        const int currentPercent = this->arbiter_.system().volume;
        int step = percent_to_steps32(currentPercent);
        if (step < 32) step++;

        // map step back to percent (1..32 -> 0..100)
        const int newPercent = (int)qRound(((step - 1) / 31.0) * 100.0);
        this->arbiter_.set_volume(newPercent);
    });

    connect(down_, &QPushButton::clicked, [this](){
        const int currentPercent = this->arbiter_.system().volume;
        int step = percent_to_steps32(currentPercent);
        if (step > 1) step--;

        const int newPercent = (int)qRound(((step - 1) / 31.0) * 100.0);
        this->arbiter_.set_volume(newPercent);
    });

    // Update label when volume changes
    connect(&this->arbiter_, &Arbiter::volume_changed, this, [this](int percent){
        this->update_label(percent);
    });

    // Initial label
    this->update_label(this->arbiter_.system().volume);
}

void RailVolume::update_label(int percent)
{
    const int step = percent_to_steps32(percent);
    label_->setText(QString::number(step));
}
