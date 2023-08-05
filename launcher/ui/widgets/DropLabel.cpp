#include "DropLabel.h"

#include <QDropEvent>
#include <QMimeData>

DropLabel::DropLabel(QWidget* parent) : QLabel(parent)
{
    setAcceptDrops(true);
}

void DropLabel::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void DropLabel::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void DropLabel::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void DropLabel::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData) {
        return;
    }

    if (mimeData->hasUrls()) {
        auto urls = mimeData->urls();
        emit droppedURLs(urls);
    }

    event->acceptProposedAction();
}
