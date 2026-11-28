#pragma once
#include <QEvent>
#include <QFont>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QRect>
#include <QSequentialAnimationGroup>
#include <QStatusBar>
#include <QWidget>

#include "CatImage.h"
#include "FaceImage.h"

// Украшения в правом нижнем углу окна:
//  - тонкая надпись "ZXC"
//  - картинка-кнопка (белый оскал); по нажатию плавно появляется фиолетовый оскал
// Без Q_OBJECT, поэтому CMake менять не нужно.
class CornerDecor : public QObject {
   public:
    explicit CornerDecor(QWidget* win) : QObject(win), m_win(win)
    {
        setObjectName("cornerDecor");

        // --- надпись ZXC ---
        m_text = new QLabel(QStringLiteral("ZXC"), win);
        QFont f;
        f.setFamilies({ QStringLiteral("Segoe UI"), QStringLiteral("Helvetica Neue"), QStringLiteral("Arial") });
        f.setPixelSize(30);
        f.setWeight(QFont::ExtraLight);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 9);
        m_text->setFont(f);
        m_text->setStyleSheet(QStringLiteral("color: rgba(255,255,255,235); background: transparent;"));
        m_text->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_text->adjustSize();

        // --- картинка-кнопка ---
        QPixmap catPm;
        catPm.loadFromData(kCatPng, kCatPngSize, "PNG");
        m_cat = new QLabel(win);
        m_cat->setPixmap(catPm);
        m_cat->setScaledContents(true);
        const int catH = 84;
        m_cat->setFixedSize(catPm.width() > 0 ? catH * catPm.width() / catPm.height() : catH, catH);
        m_cat->setCursor(Qt::PointingHandCursor);
        m_cat->setStyleSheet(QStringLiteral("background: transparent;"));
        m_catFx = new QGraphicsOpacityEffect(m_cat);
        m_catFx->setOpacity(0.0);
        m_cat->setGraphicsEffect(m_catFx);
        m_cat->installEventFilter(this);

        // --- фиолетовый оскал (появляется по клику) ---
        m_facePm.loadFromData(kFacePng, kFacePngSize, "PNG");
        m_face = new QLabel(win);
        m_face->setPixmap(m_facePm);
        m_face->setScaledContents(true);
        m_face->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_face->setStyleSheet(QStringLiteral("background: transparent;"));
        m_faceFx = new QGraphicsOpacityEffect(m_face);
        m_faceFx->setOpacity(0.0);
        m_face->setGraphicsEffect(m_faceFx);
        m_face->hide();

        auto* textFx = new QGraphicsOpacityEffect(m_text);
        textFx->setOpacity(0.0);
        m_text->setGraphicsEffect(textFx);

        win->installEventFilter(this);
        place();
        m_text->show();
        m_cat->show();
        m_text->raise();
        m_cat->raise();

        // плавное появление после приветствия
        auto* group = new QSequentialAnimationGroup(this);
        group->addPause(500);
        auto* par = new QParallelAnimationGroup(group);
        auto* a1 = new QPropertyAnimation(textFx, "opacity", par);
        a1->setDuration(1100);
        a1->setStartValue(0.0);
        a1->setEndValue(1.0);
        a1->setEasingCurve(QEasingCurve::InOutCubic);
        auto* a2 = new QPropertyAnimation(m_catFx, "opacity", par);
        a2->setDuration(1100);
        a2->setStartValue(0.0);
        a2->setEndValue(0.7);
        a2->setEasingCurve(QEasingCurve::InOutCubic);
        par->addAnimation(a1);
        par->addAnimation(a2);
        group->addAnimation(par);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

   protected:
    bool eventFilter(QObject* obj, QEvent* e) override
    {
        if (obj == m_win) {
            if (e->type() == QEvent::Resize || e->type() == QEvent::Show || e->type() == QEvent::LayoutRequest) {
                place();
            }
        } else if (obj == m_cat) {
            if (e->type() == QEvent::Enter) {
                fadeCat(1.0);
            } else if (e->type() == QEvent::Leave) {
                fadeCat(0.7);
            } else if (e->type() == QEvent::MouseButtonPress) {
                reveal();
            }
        }
        return false;
    }

   private:
    void place()
    {
        int inset = 0;
        if (auto* mw = qobject_cast<QMainWindow*>(m_win)) {
            if (mw->statusBar() && mw->statusBar()->isVisible())
                inset = mw->statusBar()->height();
        }
        const int margin = 16;
        const int bottom = m_win->height() - inset - margin;
        const int right = m_win->width() - margin;
        m_cat->move(right - m_cat->width(), bottom - m_cat->height());
        m_text->move(m_cat->x() - m_text->width() - 14, bottom - m_cat->height() / 2 - m_text->height() / 2);
        m_text->raise();
        m_cat->raise();
    }

    void fadeCat(double to)
    {
        auto* a = new QPropertyAnimation(m_catFx, "opacity", m_catFx);
        a->setDuration(220);
        a->setStartValue(m_catFx->opacity());
        a->setEndValue(to);
        a->setEasingCurve(QEasingCurve::OutCubic);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void reveal()
    {
        if (m_playing || m_facePm.isNull())
            return;
        m_playing = true;

        const int w = qBound(260, qMin(m_win->width(), m_win->height()) * 3 / 4, 640);
        const int h = w * m_facePm.height() / m_facePm.width();
        const QRect full((m_win->width() - w) / 2, (m_win->height() - h) / 2, w, h);
        const int sw = w * 7 / 10, sh = h * 7 / 10;
        const QRect small(full.center().x() - sw / 2, full.center().y() - sh / 2, sw, sh);

        m_face->setGeometry(small);
        m_faceFx->setOpacity(0.0);
        m_face->show();
        m_face->raise();

        auto* seq = new QSequentialAnimationGroup(m_face);
        auto* in = new QParallelAnimationGroup(seq);

        auto* grow = new QPropertyAnimation(m_face, "geometry", in);
        grow->setDuration(420);
        grow->setStartValue(small);
        grow->setEndValue(full);
        grow->setEasingCurve(QEasingCurve::OutBack);

        auto* fadeIn = new QPropertyAnimation(m_faceFx, "opacity", in);
        fadeIn->setDuration(320);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);

        in->addAnimation(grow);
        in->addAnimation(fadeIn);
        seq->addAnimation(in);
        seq->addPause(1300);

        auto* fadeOut = new QPropertyAnimation(m_faceFx, "opacity", seq);
        fadeOut->setDuration(650);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InOutCubic);
        seq->addAnimation(fadeOut);

        QObject::connect(seq, &QAbstractAnimation::finished, m_face, [this]() {
            m_face->hide();
            m_playing = false;
        });
        seq->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QWidget* m_win = nullptr;
    QLabel* m_text = nullptr;
    QLabel* m_cat = nullptr;
    QLabel* m_face = nullptr;
    QGraphicsOpacityEffect* m_catFx = nullptr;
    QGraphicsOpacityEffect* m_faceFx = nullptr;
    QPixmap m_facePm;
    bool m_playing = false;
};

inline void installCornerDecor(QWidget* win)
{
    if (!win || win->findChild<QObject*>(QStringLiteral("cornerDecor")))
        return;
    new CornerDecor(win);
}
