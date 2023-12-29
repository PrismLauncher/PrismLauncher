#include "IconImageProvider.h"

IconImageProvider::IconImageProvider(std::shared_ptr<IconList> iconList, int iconSize)
    : QQuickImageProvider(QQuickImageProvider::Pixmap), m_iconList(iconList), m_iconSize(iconSize)
{}

QPixmap IconImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    if (size)
        *size = QSize(m_iconSize, m_iconSize);

    QIcon i = m_iconList->getIcon(id);
    return i.pixmap(requestedSize.width() > 0 ? requestedSize.width() : m_iconSize,
                    requestedSize.height() > 0 ? requestedSize.height() : m_iconSize);
}
