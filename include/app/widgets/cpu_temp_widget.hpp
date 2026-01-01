#pragma once

#include <QWidget>

class QLabel;
class QTimer;

class CpuTempWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CpuTempWidget(QWidget *parent = nullptr);

private:
    QLabel *label_ = nullptr;
    QTimer *timer_ = nullptr;

    static double read_cpu_temp_c();
    void applyTempState(const char *state);
    void updateTemp();
};
