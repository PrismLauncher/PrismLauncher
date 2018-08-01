#pragma once

#include <QSet>
#include <QString>
#include <QFile>
#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT FtbPrivatePackManager
{
public:
    ~FtbPrivatePackManager()
    {
        save();
    }
    void load();
    void save() const;
    bool empty() const
    {
        return currentPacks.empty();
    }
    const QSet<QString> &getCurrentPackCodes() const
    {
        return currentPacks;
    }
    void add(const QString &code)
    {
        currentPacks.insert(code);
        dirty = true;
    }
    void remove(const QString &code)
    {
        currentPacks.remove(code);
        dirty = true;
    }

private:
    QSet<QString> currentPacks;
    QString m_filename = "private_packs.txt";
    mutable bool dirty = false;
};
