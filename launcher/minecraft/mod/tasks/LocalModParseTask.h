#pragma once

#include <QDebug>
#include <QObject>

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModDetails.h"

#include "tasks/Task.h"

namespace ModUtils {

ModDetails ReadFabricModInfo(const QByteArray& contents);
ModDetails ReadQuiltModInfo(const QByteArray& contents);
ModDetails ReadForgeInfo(const QByteArray& contents);
ModDetails ReadLiteModInfo(const QByteArray& contents);

bool process(Mod& mod);

bool processZIP(Mod& mod);
bool processLitemod(Mod& mod);

/** Checks whether a file is valid as a mod or not. */
bool validate(QFileInfo file);

QPixmap loadIconFile(ResourceType type, const QString& path, const QString& iconPath);
}  // namespace ModUtils

class LocalModParseTask : public Task {
    Q_OBJECT
   public:
    struct Result {
        ModDetails details;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const { return m_result; }

    bool canAbort() const override { return true; }
    bool abort() override;

    LocalModParseTask(int token, ResourceType type, const QFileInfo& modFile);
    void executeTask() override;

    int token() const { return m_token; }

   private:
    int m_token;
    ResourceType m_type;
    QFileInfo m_modFile;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;
};
