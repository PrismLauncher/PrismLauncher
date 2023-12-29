#pragma once
#include "IconList.h"

#include <QQuickImageProvider>

class IconImageProvider : public QQuickImageProvider {
   public:
    IconImageProvider(std::shared_ptr<IconList> iconList, int iconSize = 48);

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

   private:
    std::shared_ptr<IconList> m_iconList;
    int m_iconSize;
};
