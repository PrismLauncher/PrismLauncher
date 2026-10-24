#pragma once

#include <QCryptographicHash>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <utility>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

namespace Hashing {

enum class Algorithm : std::uint8_t { Md4, Md5, Sha1, Sha256, Sha512, Murmur2, Unknown };

QString algorithmToString(Algorithm type);
Algorithm algorithmFromString(const QString& type);
QString hash(QIODevice* device, Algorithm type);
QString hash(const QString& fileName, Algorithm type);
QString hash(const QByteArray& data, Algorithm type);

class Hasher : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Hasher>;

    Hasher(QString filePath, Algorithm alg) : m_path(std::move(filePath)), m_alg(alg) {}
    Hasher(QString filePath, const QString& alg) : Hasher(std::move(filePath), algorithmFromString(alg)) {}

    bool abort() override;

    void executeTask() override;

    QString getResult() const { return m_result; };
    QString getPath() const { return m_path; };

   signals:
    void resultsReady(QString hash);

   private:
    QString m_result;
    QString m_path;
    Algorithm m_alg;

    QFuture<QString> m_future;
    QFutureWatcher<QString> m_watcher;
};

Hasher::Ptr createHasher(QString filePath, ModPlatform::ResourceProvider provider);
Hasher::Ptr createHasher(QString filePath, const QString& type);

}  // namespace Hashing
