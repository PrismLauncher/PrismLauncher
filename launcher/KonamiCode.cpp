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
        if(key == konamiCode[hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress])
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress ++;
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress = 0;
        }
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress == static_cast<int>(konamiCode.size()))
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress = 0;
            emit triggered();
        }
    }
}
