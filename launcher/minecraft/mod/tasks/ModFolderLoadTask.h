#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QRunnable>
#include <memory>
#include "minecraft/mod/Mod.h"

class ModFolderLoadTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    struct Result {
        QMap<QString, Mod> mods;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const {
        return m_result;
    }

public:
    ModFolderLoadTask(QDir& mods_dir, QDir& index_dir);
    void run();
signals:
    void succeeded();

private:
    void getFromMetadata();

private:
    QDir& m_mods_dir, m_index_dir;
    ResultPtr m_result;
};
