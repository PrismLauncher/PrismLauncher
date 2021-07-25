#pragma once

#include <QSet>
#include <QString>
#include <QFile>

namespace LegacyFTB {

class PrivatePackManager
{
public:
    ~PrivatePackManager()
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

}
