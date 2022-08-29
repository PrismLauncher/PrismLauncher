#pragma once

#include "Resource.h"

#include <QMutex>

class Version;

/* TODO:
 *
 * Store pack.png
 * Store localized descriptions
 * */

class ResourcePack : public Resource {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    ResourcePack(QObject* parent = nullptr) : Resource(parent) {}
    ResourcePack(QFileInfo file_info) : Resource(file_info) {}

    /** Gets the numerical ID of the pack format. */
    [[nodiscard]] int packFormat() const { return m_pack_format; }
    /** Gets, respectively, the lower and upper versions supported by the set pack format. */
    [[nodiscard]] std::pair<Version, Version> compatibleVersions() const;

    /** Gets the description of the resource pack. */
    [[nodiscard]] QString description() const { return m_description; }

    /** Thread-safe. */
    void setPackFormat(int new_format_id);

    /** Thread-safe. */
    void setDescription(QString new_description);

    [[nodiscard]] auto compare(Resource const& other, SortType type) const -> std::pair<int, bool> override;
    [[nodiscard]] bool applyFilter(QRegularExpression filter) const override;

   protected:
    mutable QMutex m_data_lock;

    /* The 'version' of a resource pack, as defined in the pack.mcmeta file.
     * See https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
     */
    int m_pack_format = 0;

    /** The resource pack's description, as defined in the pack.mcmeta file.
     */
    QString m_description;
};
