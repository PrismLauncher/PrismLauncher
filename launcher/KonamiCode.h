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
    int m_progress = 0;
};
