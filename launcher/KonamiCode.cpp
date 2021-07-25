#include "KonamiCode.h"

#include <array>
#include <QDebug>

namespace {
const std::array<Qt::Key, 10> konamiCode =
{
    {
        Qt::Key_Up, Qt::Key_Up,
        Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Right,
        Qt::Key_Left, Qt::Key_Right,
        Qt::Key_B, Qt::Key_A
    }
};
}

KonamiCode::KonamiCode(QObject* parent) : QObject(parent)
{
}


void KonamiCode::input(QEvent* event)
{
    if( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
        auto key = Qt::Key(keyEvent->key());
        if(key == konamiCode[m_progress])
        {
            m_progress ++;
        }
        else
        {
            m_progress = 0;
        }
        if(m_progress == static_cast<int>(konamiCode.size()))
        {
            m_progress = 0;
            emit triggered();
        }
    }
}
