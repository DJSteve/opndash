#pragma once

#include <QButtonGroup>
#include <QKeyEvent>
#include <QMainWindow>
#include <QObject>
#include <QShowEvent>
#include <QStackedLayout>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QStackedWidget>

#include "app/config.hpp"
#include "app/pages/openauto.hpp"
#include "app/pages/page.hpp"

#include "app/arbiter.hpp"
#include "app/widgets/nav_neon_indicator.hpp"


class FullscreenToggle;

class NavNeonIndicator;

class QGraphicsOpacityEffect;
class QPropertyAnimation;



class Dash : public QWidget {
    Q_OBJECT


   public:
    Dash(Arbiter &arbiter);
    void init();

   private:
    struct NavRail {
        QButtonGroup group;
        QElapsedTimer timer;
	QWidget *widget;
        QVBoxLayout *layout;
        NavRail();
    };

    struct Body {
        QVBoxLayout *layout;
        QVBoxLayout *status_bar;
        QStackedLayout *frame;
	QWidget *control_bar_widget;
        QVBoxLayout *control_bar;
	QWidget *frame_widget;

        Body();
    };

    Arbiter &arbiter;
    NavRail rail;
    Body body;

    
    // Transition overlay (safe across OpenGL/video pages)
    QWidget *transition_overlay = nullptr;
    QGraphicsOpacityEffect *transition_fx = nullptr;
    QPropertyAnimation *transition_anim = nullptr;
    bool transitioning = false;
    Page *pending_page = nullptr;
    NavNeonIndicator *nav_neon = nullptr;


void set_page(Page *page);
    QWidget *status_bar() const;
    QWidget *control_bar() const;
    QWidget *power_control() const;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QRect geometry);
    void set_fullscreen(Page *page);

   protected:
    void showEvent(QShowEvent *event) override;

   private:
    Arbiter arbiter;
    QStackedWidget *stack;

    MainWindow *init(QRect geometry);
};
