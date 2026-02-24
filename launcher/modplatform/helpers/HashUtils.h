#pragma once

#include <QCryptographicHash>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <utility>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

namespace Hashing {

enum class Algorithm { Md4, Md5, Sha1, Sha256, Sha512, Murmur2, Unknown };

QString algorithmToString(Algorithm type);
Algorithm algorithmFromString(const QString& type);
QString hash(QIODevice* device, Algorithm type);
QString hash(const QString& fileName, Algorithm type);
QString hash(QByteArray data, Algorithm type);

class Hasher : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Hasher>;

    Hasher(QString file_path, Algorithm alg) : m_path(std::move(file_path)), m_alg(alg) {}
    Hasher(QString file_path, QString alg) : Hasher(std::move(file_path), algorithmFromString(std::move(alg))) {}

    bool abort() override;

    void executeTask() override;

    QString getResult() const { return m_result; };
    QString getPath() const { return m_path; };

   signals:
    void resultsReady(const QString& hash);

   private:
    QString m_result;
    QString m_path;
    Algorithm m_alg;

    QFuture<QString> m_future;
    QFutureWatcher<QString> m_watcher;
};

Hasher::Ptr createHasher(const QString& file_path, ModPlatform::ResourceProvider provider);
Hasher::Ptr createHasher(QString file_path, QString type);

}  // namespace Hashing
