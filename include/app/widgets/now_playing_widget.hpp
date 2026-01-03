#pragma once

#include <QWidget>

class QLabel;
class QTimer;

class NowPlayingWidget : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingWidget(QWidget *parent = nullptr);

    void setSourceTag(const QString &tag);   // "BT", "AA", "LOCAL"
    void setNowPlaying(const QString &text); // "Artist — Title"
    void setActive(bool active);             // dims when inactive

private:
    QLabel *tag_ = nullptr;
    QLabel *text_ = nullptr;

    // simple marquee
    QTimer *marquee_ = nullptr;
    QString full_;
    int offset_ = 0;
    void refreshMarquee();
};
