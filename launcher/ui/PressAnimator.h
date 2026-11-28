#pragma once
#include <QAbstractAnimation>
#include <QAbstractButton>
#include <QComboBox>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QObject>
#include <QPropertyAnimation>
#include <QTabBar>
#include <QWidget>

// Плавная реакция на мышь для ВСЕХ окон (в том числе окна консоли):
//  - кнопки (QAbstractButton: кнопки, галочки, переключатели): плавное затемнение при наведении,
//    мягкое "вдавливание" при нажатии и долгое плавное возвращение
//  - вкладки (QTabBar) и выпадающие списки (QComboBox): плавный "пульс" при нажатии
// Без Q_OBJECT, поэтому CMake менять не нужно.
class PressAnimator : public QObject {
   public:
    explicit PressAnimator(QObject* parent = nullptr) : QObject(parent) {}

   protected:
    bool eventFilter(QObject* obj, QEvent* e) override
    {
        const QEvent::Type t = e->type();
        if (t != QEvent::MouseButtonPress && t != QEvent::MouseButtonRelease && t != QEvent::Enter && t != QEvent::Leave)
            return false;
        if (!obj->isWidgetType())
            return false;

        auto* w = static_cast<QWidget*>(obj);
        const bool isButton = qobject_cast<QAbstractButton*>(w) != nullptr;
        const bool isOther = qobject_cast<QTabBar*>(w) != nullptr || qobject_cast<QComboBox*>(w) != nullptr;
        if (!isButton && !isOther)
            return false;
        if (!w->isEnabled())
            return false;

        switch (t) {
            case QEvent::MouseButtonPress:
                animate(w, isButton ? 0.40 : 0.60, 140, QEasingCurve::OutQuad);
                break;
            case QEvent::MouseButtonRelease:
                animate(w, (isButton && w->underMouse()) ? 0.88 : 1.0, 560, QEasingCurve::OutCubic);
                break;
            case QEvent::Enter:
                if (isButton && QGuiApplication::mouseButtons() == Qt::NoButton)
                    animate(w, 0.88, 280, QEasingCurve::OutCubic);
                break;
            case QEvent::Leave:
                if (isButton || isOther)
                    animate(w, 1.0, 380, QEasingCurve::OutCubic);
                break;
            default:
                break;
        }
        return false;
    }

   private:
    static void animate(QWidget* w, double to, int ms, QEasingCurve::Type curve)
    {
        auto* fx = qobject_cast<QGraphicsOpacityEffect*>(w->graphicsEffect());
        if (!fx) {
            if (w->graphicsEffect())
                return;  // не трогаем чужие эффекты
            fx = new QGraphicsOpacityEffect(w);
            fx->setOpacity(1.0);
            w->setGraphicsEffect(fx);
            fx->setEnabled(false);
        }

        auto* a = fx->findChild<QPropertyAnimation*>(QStringLiteral("pressAnim"));
        if (!a) {
            a = new QPropertyAnimation(fx, "opacity", fx);
            a->setObjectName(QStringLiteral("pressAnim"));
            // когда вернулись к полной непрозрачности, выключаем эффект (чтобы текст оставался чётким)
            QObject::connect(a, &QAbstractAnimation::finished, fx, [fx]() {
                if (fx->opacity() >= 0.999)
                    fx->setEnabled(false);
            });
        }
        a->stop();
        fx->setEnabled(true);
        a->setDuration(ms);
        a->setEasingCurve(curve);
        a->setStartValue(fx->opacity());
        a->setEndValue(to);
        a->start();
    }
};
