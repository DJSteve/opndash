#pragma once
#include <QWidget>
#include <QPropertyAnimation>

class NavNeonIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int yPos READ yPos WRITE setYPos)

public:
    explicit NavNeonIndicator(QWidget *parent = nullptr);

    int yPos() const { return this->y_pos; }
    void setYPos(int y);

    void moveToButton(QWidget *button, bool animate = true);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int y_pos = 0;
    int h = 56;          // will be updated from button height
    int margin = 8;      // inset from rail edges

    QPropertyAnimation *anim = nullptr;

    // theme-ish colours (tweak later to match your exact palette)
    QColor glowOuter = QColor(140, 40, 255, 140); // purple
    QColor glowInner = QColor(0, 220, 255, 180);  // cyan
    QColor fill      = QColor(20, 10, 35, 80);    // dark translucent fill
};
