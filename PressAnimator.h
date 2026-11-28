#pragma once
#include <QAbstractButton>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QObject>
#include <QPropertyAnimation>

// Плавное затемнение кнопок при нажатии. Без Q_OBJECT, поэтому CMake менять не нужно.
class PressAnimator : public QObject {
   public:
    explicit PressAnimator(QObject* parent = nullptr) : QObject(parent) {}

   protected:
    bool eventFilter(QObject* obj, QEvent* e) override
    {
        auto* b = qobject_cast<QAbstractButton*>(obj);
        if (!b || (e->type() != QEvent::MouseButtonPress && e->type() != QEvent::MouseButtonRelease))
            return false;

        bool press = e->type() == QEvent::MouseButtonPress;
        auto* fx = qobject_cast<QGraphicsOpacityEffect*>(b->graphicsEffect());
        if (!fx) {
            if (b->graphicsEffect())
                return false;
            fx = new QGraphicsOpacityEffect(b);
            b->setGraphicsEffect(fx);
        }
        auto* a = new QPropertyAnimation(fx, "opacity", fx);
        a->setDuration(press ? 90 : 240);
        a->setEasingCurve(QEasingCurve::OutCubic);
        a->setStartValue(fx->opacity());
        a->setEndValue(press ? 0.55 : 1.0);
        a->start(QAbstractAnimation::DeleteWhenStopped);
        return false;
    }
};
