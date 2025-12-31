#include "app/widgets/rail_volume.hpp"

#include "app/arbiter.hpp"
#include "app/session.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QtMath>
#include <QTimer>
#include <QEvent>

static QColor volumeColor(int value)
{
    // Clamp just in case
    value = qBound(1, value, 32);

    // Normalize 0.0 → 1.0
    const float t = (value - 1) / 31.0f;

    QColor low(0, 220, 220);     // cyan
    QColor mid(160, 80, 255);    // purple
    QColor high(255, 80, 80);    // red

    QColor a, b;
    float localT;

    if (t < 0.5f) {
        a = low;
        b = mid;
        localT = t * 2.0f;
    } else {
        a = mid;
        b = high;
        localT = (t - 0.5f) * 2.0f;
    }

    return QColor(
        a.red()   + (b.red()   - a.red())   * localT,
        a.green() + (b.green() - a.green()) * localT,
        a.blue()  + (b.blue()  - a.blue())  * localT
    );
}

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
    up_->setCheckable(false);
    down_->setCheckable(false);
    up_->setAutoRepeat(true);
    down_->setAutoRepeat(true);
    up_->setAutoRepeatInterval(60);
    down_->setAutoRepeatInterval(60);

    // Use your existing icons (resource names must match your .qrc)
    // If your icons are named differently, swap these strings.
    QIcon upIcon(QString(":/icons/volume_up.svg"));
    QIcon downIcon(QString(":/icons/volume_down.svg"));
    up_->setIcon(upIcon);
    down_->setIcon(downIcon);

    // Match sidebar button sizing
    const QSize buttonSize(48, 48);
    const QSize iconSize(48, 48);

    up_->setFixedSize(buttonSize);
    down_->setFixedSize(buttonSize);

    up_->setIconSize(iconSize);
    down_->setIconSize(iconSize);

    // Label
    label_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label_->setText("1"); // temporary until signal arrives
    label_->setCursor(Qt::PointingHandCursor);
    label_->setToolTip("Tap to mute");
    label_->installEventFilter(this);
    label_->setMinimumWidth(40);


    flashTimer_ = new QTimer(this);
    flashTimer_->setInterval(450);

    connect(flashTimer_, &QTimer::timeout, this, [this]{
    if (!muted_) return;
    	flashOn_ = !flashOn_;
    	// just re-render the label
    	this->update_label(this->arbiter_.system().volume);
    });

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

if (percent > 0 && muted_) {
    muted_ = false;
    flashTimer_->stop();
    flashOn_ = false;
}

    // detect mute state from percent too (if something else sets volume 0)
    if (percent <= 0) muted_ = true;

    if (muted_) {
        label_->setText("MUTE");

        // Flash between bright red and dim red
        if (flashOn_) {
            label_->setStyleSheet("color: rgb(255,80,80); font-weight: 700;");
        } else {
            label_->setStyleSheet("color: rgba(255,80,80,120); font-weight: 700;");
        }
        return;
    }

    const int step = percent_to_steps32(percent);
    label_->setText(QString::number(step));
    QColor c = volumeColor(step);
    label_->setStyleSheet(
    QString("color: rgb(%1,%2,%3);")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
    );
}

void RailVolume::set_muted(bool on)
{
    if (on == muted_) return;

    muted_ = on;

    if (muted_) {
        // store last non-zero percent for restore
        const int current = this->arbiter_.system().volume;
        if (current > 0) last_percent_ = current;

        flashOn_ = true;
        flashTimer_->start();

        this->arbiter_.set_volume(0);
    } else {
        flashTimer_->stop();
        flashOn_ = false;

        // restore to last saved level (fallback to 50 if weird)
        int restore = last_percent_;
        if (restore < 1) restore = 50;
        this->arbiter_.set_volume(restore);
    }
}

bool RailVolume::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == label_ && event->type() == QEvent::MouseButtonPress) {
        this->set_muted(!this->muted_);
        return true; // handled
    }
    return QWidget::eventFilter(obj, event);
}
