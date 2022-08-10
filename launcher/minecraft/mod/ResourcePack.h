#pragma once

#include "Resource.h"

class ResourcePack : public Resource {
    Q_OBJECT
    public:
    using Ptr = shared_qobject_ptr<Resource>;

    ResourcePack(QObject* parent = nullptr) : Resource(parent) {}
    ResourcePack(QFileInfo file_info) : Resource(file_info) {}

};
