#pragma once

#include <QString>
#include <QStringList>

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
};
