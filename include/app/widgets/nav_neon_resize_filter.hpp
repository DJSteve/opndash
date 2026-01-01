#pragma once

#include <QObject>
#include <QEvent>
#include <QWidget>

#include "app/widgets/nav_neon_indicator.hpp"

class NavNeonResizeFilter : public QObject {
public:
    NavNeonResizeFilter(NavNeonIndicator* neon, QWidget* rail)
        : QObject(rail), neon_(neon), rail_(rail) {}

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == rail_ && ev->type() == QEvent::Resize) {
            if (neon_) neon_->setGeometry(rail_->rect());
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    NavNeonIndicator* neon_;
    QWidget* rail_;
};
