#pragma once

#include <QString>
#include <QStringList>

enum IconType : unsigned
{
    Builtin,
    Transient,
    FileBased,
    ICONS_TOTAL,
    ToBeDeleted
};

class IIconList
{
public:
    virtual ~IIconList();
    virtual bool addIcon(const QString &key, const QString &name, const QString &path, const IconType type) = 0;
    virtual bool deleteIcon(const QString &key) = 0;
    virtual void saveIcon(const QString &key, const QString &path, const char * format) const = 0;
    virtual bool iconFileExists(const QString &key) const = 0;
    virtual void installIcons(const QStringList &iconFiles) = 0;
    virtual void installIcon(const QString &file, const QString &name) = 0;
};
