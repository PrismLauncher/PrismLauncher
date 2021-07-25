#pragma once
#include <QRunnable>
#include <QObject>
#include <QDir>
#include <QMap>
#include "Mod.h"
#include <memory>

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
    ModFolderLoadTask(QDir dir);
    void run();
signals:
    void succeeded();
private:
    QDir m_dir;
    ResultPtr m_result;
};
