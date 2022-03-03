#pragma once

#include <RWStorage.h>

#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QThreadPool>

#include <net/NetJob.h>
#include <functional>

#include "BaseInstance.h"
#include "ModrinthPage.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace Modrinth {

typedef std::function<void(QString)> LogoCallback;

class ListModel : public ModPlatform::ListModel {
    Q_OBJECT

   public:
    ListModel(ModrinthPage* parent);
    virtual ~ListModel();

   private slots:
    void performPaginatedSearch() override;
    void searchRequestFinished() override;
};

}  // namespace Modrinth
