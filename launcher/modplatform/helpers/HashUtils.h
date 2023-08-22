#pragma once

#include <QString>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

namespace Hashing {

class Hasher : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Hasher>;

    Hasher(QString file_path) : m_path(std::move(file_path)) {}

    /* We can't really abort this task, but we can say we aborted and finish our thing quickly :) */
    bool abort() override { return true; }

    void executeTask() override = 0;

    QString getResult() const { return m_hash; };
    QString getPath() const { return m_path; };

   signals:
    void resultsReady(QString hash);

   protected:
    QString m_hash;
    QString m_path;
};

class FlameHasher : public Hasher {
   public:
    FlameHasher(QString file_path) : Hasher(file_path) { setObjectName(QString("FlameHasher: %1").arg(file_path)); }

    void executeTask() override;
};

class ModrinthHasher : public Hasher {
   public:
    ModrinthHasher(QString file_path) : Hasher(file_path) { setObjectName(QString("ModrinthHasher: %1").arg(file_path)); }

    void executeTask() override;
};

class BlockedModHasher : public Hasher {
   public:
    BlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider);

    void executeTask() override;

    QStringList getHashTypes();
    bool useHashType(QString type);

   private:
    ModPlatform::ResourceProvider provider;
    QString hash_type;
};

Hasher::Ptr createHasher(QString file_path, ModPlatform::ResourceProvider provider);
Hasher::Ptr createFlameHasher(QString file_path);
Hasher::Ptr createModrinthHasher(QString file_path);
Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider);
Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider, QString type);

}  // namespace Hashing
