#pragma once

#include <QPushButton>

class Arbiter;
class QTimer;

class ControlBluetoothButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ControlBluetoothButton(Arbiter &arbiter, QWidget *parent = nullptr, int iconPx = 32);

private:
    Arbiter &arbiter_;

    bool scanning_ = false;
    QTimer *pulseTimer_ = nullptr;
    int iconPx_ = 32;

    void refresh();
    void setBtState(const char *state);
    void setPulse(int v);
    QString buildTooltip() const;
};
