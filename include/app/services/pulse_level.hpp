#pragma once

#include <QObject>
#include <atomic>

class PulseLevel : public QObject
{
    Q_OBJECT
public:
    explicit PulseLevel(QObject *parent = nullptr);
    ~PulseLevel() override;

    void start();
    void stop();

signals:
    void level(float v); // 0..1

private:
    std::atomic<bool> running_{false};
    void run(); // thread entry
};
