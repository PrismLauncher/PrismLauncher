#pragma once

#include <QDebug>
#include <QObject>

#include "minecraft/mod/ResourcePack.h"

#include "tasks/Task.h"

class LocalResourcePackParseTask : public Task {
    Q_OBJECT
   public:
    LocalResourcePackParseTask(int token, ResourcePack& rp);

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    void executeTask() override;

    [[nodiscard]] int token() const { return m_token; }

   private:
    void processMCMeta(QByteArray&& raw_data);

    void processAsFolder();
    void processAsZip();

   private:
    int m_token;

    ResourcePack& m_resource_pack;

    bool m_aborted = false;
};
