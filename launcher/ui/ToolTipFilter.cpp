#include "ToolTipFilter.h"

bool ToolTipFilter::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::ToolTip) {
        return true;
    } else {
        return QObject::eventFilter(obj, ev);
    }
}
