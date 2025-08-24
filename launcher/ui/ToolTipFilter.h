#pragma once

#include <qobject.h>
#include <qevent.h>

class ToolTipFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};
