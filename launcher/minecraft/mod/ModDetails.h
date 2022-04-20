#pragma once

#include <memory>

#include <QString>
#include <QStringList>

#include "minecraft/mod/MetadataHandler.h"

enum class ModStatus {
    Installed,      // Both JAR and Metadata are present
    NotInstalled,   // Only the Metadata is present
    NoMetadata,     // Only the JAR is present
};

struct ModDetails
{
    /* Mod ID as defined in the ModLoader-specific metadata */
    QString mod_id;
    
    /* Human-readable name */
    QString name;
    
    /* Human-readable mod version */
    QString version;
    
    /* Human-readable minecraft version */
    QString mcversion;
    
    /* URL for mod's home page */
    QString homeurl;
    
    /* Human-readable description */
    QString description;

    /* List of the author's names */
    QStringList authors;

    /* Installation status of the mod */
    ModStatus status;

    /* Metadata information, if any */
    std::shared_ptr<Metadata::ModStruct> metadata;
};
