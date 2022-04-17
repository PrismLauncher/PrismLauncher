#pragma once

#include <memory>

#include <QString>
#include <QStringList>

#include "minecraft/mod/MetadataHandler.h"

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

    /* Metadata information, if any */
    std::shared_ptr<Metadata::ModStruct> metadata;
};
