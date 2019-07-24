#pragma once

#include <QToolBar>
#include <QAction>
#include <QMap>

class QMenu;

class WideBar : public QToolBar
{
    Q_OBJECT

public:
    explicit WideBar(const QString &title, QWidget * parent = nullptr);
    explicit WideBar(QWidget * parent = nullptr);
    virtual ~WideBar();

    void addAction(QAction *action);
    void addSeparator();
    void insertSpacer(QAction *action);
    QMenu *createContextMenu(QWidget *parent = nullptr, const QString & title = QString());

private:
    struct BarEntry;
    QList<BarEntry *> m_entries;
};
