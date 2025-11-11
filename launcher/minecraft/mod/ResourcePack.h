#pragma once

#include "Resource.h"
#include "minecraft/mod/DataPack.h"

#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

class Version;

/* TODO:
 *
 * Store localized descriptions
 * */

class ResourcePack : public DataPack {
    Q_OBJECT
   public:
    ResourcePack(QObject* parent = nullptr) : DataPack(parent) {}
    ResourcePack(QFileInfo file_info) : DataPack(file_info) {}

    /** Gets, respectively, the lower and upper versions supported by the set pack format. */
    std::pair<Version, Version> compatibleVersions() const override;
};
