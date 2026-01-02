#pragma once

#include <QPushButton>

class Arbiter;
class Dialog;

class ControlPowerButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ControlPowerButton(Arbiter &arbiter, QWidget *parent = nullptr, int iconPx = 32);

private:
    Arbiter &arbiter_;
    Dialog *dialog_ = nullptr;

    QWidget *buildPowerControl() const;
    void ensureDialog();
};
