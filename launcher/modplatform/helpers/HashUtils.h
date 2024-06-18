#pragma once

#include <QCryptographicHash>
#include <QString>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

namespace Hashing {

enum class Algorithm { Md4, Md5, Sha1, Sha256, Sha512, Murmur2, Unknown };

QString algorithmToString(Algorithm type);
Algorithm algorithmFromString(QString type);
QString hash(QIODevice* device, Algorithm type);
QString hash(QString fileName, Algorithm type);
QString hash(QByteArray data, Algorithm type);

class Hasher : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Hasher>;

    Hasher(QString file_path, Algorithm alg) : m_path(file_path), m_alg(alg) {}
    Hasher(QString file_path, QString alg) : Hasher(file_path, algorithmFromString(alg)) {}

    /* We can't really abort this task, but we can say we aborted and finish our thing quickly :) */
    bool abort() override { return true; }

    void executeTask() override;

    QString getResult() const { return m_result; };
    QString getPath() const { return m_path; };

   signals:
    void resultsReady(QString hash);

   protected:
    QString m_result;
    QString m_path;
    Algorithm m_alg;
};

Hasher::Ptr createHasher(QString file_path, ModPlatform::ResourceProvider provider);
Hasher::Ptr createHasher(QString file_path, QString type);

}  // namespace Hashing
