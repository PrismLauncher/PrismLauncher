#pragma once

#include <QKeyEvent>

class KonamiCode : public QObject
{
    Q_OBJECT
public:
    KonamiCode(QObject *parent = 0);
    void input(QEvent *event);

signals:
    void triggered();

private:
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_progress = 0;
};
