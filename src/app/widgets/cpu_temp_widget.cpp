#include "app/widgets/cpu_temp_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QFile>
#include <QString>
#include <QStyle>
#include <QVariant>

CpuTempWidget::CpuTempWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    label_ = new QLabel("--.-°C", this);
    label_->setObjectName("CpuTemp");
    label_->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    label_->setMinimumWidth(70);
    label_->setProperty("temp_state", QVariant(QStringLiteral("unknown")));

    layout->addWidget(label_);

    timer_ = new QTimer(this);
    timer_->setInterval(1000);

    connect(timer_, &QTimer::timeout, this, [this]{
        updateTemp();
    });

    timer_->start();
    updateTemp(); // run immediately
}

double CpuTempWidget::read_cpu_temp_c()
{
    // Fast + zero shell: sysfs
    QFile f("/sys/class/thermal/thermal_zone0/temp");
    if (!f.open(QIODevice::ReadOnly))
        return -1.0;

    const QByteArray data = f.readAll().trimmed();
    bool ok = false;
    const int milli = data.toInt(&ok);
    if (!ok) return -1.0;
    return milli / 1000.0;
}

void CpuTempWidget::applyTempState(const char *state)
{
    label_->setProperty(
        "temp_state",
        QVariant(QString::fromLatin1(state))
    );

    label_->style()->unpolish(label_);
    label_->style()->polish(label_);
    label_->update();
}

void CpuTempWidget::updateTemp()
{
    const double t = read_cpu_temp_c();

    if (t < 0.0) {
        label_->setText("--.-°C");
        applyTempState("unknown");
        return;
    }

    label_->setText(QString::asprintf("%.1f°C", t));

    if (t < 60.0)       applyTempState("cool");
    else if (t < 70.0)  applyTempState("normal");
    else if (t < 80.0)  applyTempState("warm");
    else                applyTempState("hot");
}
