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
    void onClip(bool left, bool right);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
std::atomic<float> levelL_{0.0f};
std::atomic<float> levelR_{0.0f};
float displayL_{0.0f};
float displayR_{0.0f};
float peakHoldL_{0.0f};
float peakHoldR_{0.0f};

// --- CLIP latch ---
int clipTicksL_{0};
int clipTicksR_{0};
static constexpr float kClipThreshold = 0.98f;
static constexpr int kClipHoldTicks = 48; // 16 * 50ms ≈ 800ms

    QTimer *tick_ = nullptr;

    AlsaLevel *pulse_ = nullptr;
};
