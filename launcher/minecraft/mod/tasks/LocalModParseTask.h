#pragma once

#include <QDebug>
#include <QObject>

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModDetails.h"

#include "tasks/Task.h"

namespace ModUtils {

ModDetails ReadFabricModInfo(QByteArray contents);
ModDetails ReadQuiltModInfo(QByteArray contents);
ModDetails ReadForgeInfo(QByteArray contents);
ModDetails ReadLiteModInfo(QByteArray contents);

bool process(Mod& mod);

bool processZIP(Mod& mod);
bool processFolder(Mod& mod);
bool processLitemod(Mod& mod);

/** Checks whether a file is valid as a mod or not. */
bool validate(QFileInfo file);

bool processIconPNG(const Mod& mod, QByteArray&& raw_data, QPixmap* pixmap);
bool loadIconFile(const Mod& mod, QPixmap* pixmap);
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

    LocalModParseTask(int token, const QFileInfo& modFile);
    void executeTask() override;

    int token() const { return m_token; }

   private:
    int m_token;
    QFileInfo m_modFile;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;
};
