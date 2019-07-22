#pragma once

#include <QToolBar>

class WideBar : public QToolBar
{
    Q_OBJECT

public:
    explicit WideBar(const QString &title, QWidget * parent = nullptr);
    explicit WideBar(QWidget * parent = nullptr);

    void addAction(QAction *action);
    void insertSpacer(QAction *action);

private:
    QMap<QAction *, QAction *> m_actionMap;
};
