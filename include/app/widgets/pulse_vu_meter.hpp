#pragma once

#include <QWidget>
#include <atomic>

class QTimer;
class AlsaLevel;

class PulseVUMeter : public QWidget
{
    Q_OBJECT
public:
    explicit PulseVUMeter(QWidget *parent = nullptr);
    ~PulseVUMeter() override;

    QSize sizeHint() const override { return QSize(56, 220); }

public slots:
    void setLevel(float left, float right); // 0..1

protected:
    void paintEvent(QPaintEvent *e) override;

private:
std::atomic<float> levelL_{0.0f};
std::atomic<float> levelR_{0.0f};
float displayL_{0.0f};
float displayR_{0.0f};
float peakHoldL_{0.0f};
float peakHoldR_{0.0f};

    QTimer *tick_ = nullptr;

    AlsaLevel *pulse_ = nullptr;
};
