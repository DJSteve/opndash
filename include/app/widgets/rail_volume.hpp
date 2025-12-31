#pragma once

#include <QWidget>
#include <QEvent>

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
    bool muted_ = false;
    int last_percent_ = 50; // restore target when unmuting
    class QTimer *flashTimer_ = nullptr;
    bool flashOn_ = false;

    void set_muted(bool on);
    void update_label(int percent);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
