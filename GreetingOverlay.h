#pragma once
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QSequentialAnimationGroup>
#include <QWidget>

// Плавное приветствие: появление -> пауза -> исчезновение.
inline void showGreeting(QWidget* parent)
{
    auto* greet = new QLabel(QString::fromUtf8("Добрый день, почемв!"), parent);
    greet->setStyleSheet(
        "color: white; font-size: 34px; font-weight: 600;"
        "padding: 22px 40px; border-radius: 16px;"
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #000, stop:1 #555);"
        "border: 1px solid #aaa;");
    greet->setAttribute(Qt::WA_TransparentForMouseEvents);
    greet->adjustSize();
    greet->move((parent->width() - greet->width()) / 2, (parent->height() - greet->height()) / 2);

    auto* fx = new QGraphicsOpacityEffect(greet);
    fx->setOpacity(0.0);
    greet->setGraphicsEffect(fx);

    auto* fadeIn = new QPropertyAnimation(fx, "opacity");
    fadeIn->setDuration(900);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutCubic);

    auto* fadeOut = new QPropertyAnimation(fx, "opacity");
    fadeOut->setDuration(900);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutCubic);

    auto* seq = new QSequentialAnimationGroup(greet);
    seq->addAnimation(fadeIn);
    seq->addPause(1500);
    seq->addAnimation(fadeOut);
    QObject::connect(seq, &QAbstractAnimation::finished, greet, &QObject::deleteLater);

    greet->show();
    greet->raise();
    seq->start();
}
