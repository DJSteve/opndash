#pragma once

#include <QObject>
#include <atomic>

class AlsaLevel : public QObject
{
    Q_OBJECT
public:
    explicit AlsaLevel(QObject *parent = nullptr);
    ~AlsaLevel() override;

    void start();
    void stop();

signals:
    void level(float left, float right); // 0..1 each

private:
    std::atomic<bool> running_{false};
    void run(); // thread entry
};
