#pragma once

#include <QWidget>

class Arbiter;
class QLabel;
class QPushButton;

class RailVolume : public QWidget
{
    Q_OBJECT

public:
    explicit RailVolume(Arbiter &arbiter, QWidget *parent = nullptr);

private:
    Arbiter &arbiter_;
    QLabel *label_;
    QPushButton *up_;
    QPushButton *down_;

    void update_label(int percent);
};
