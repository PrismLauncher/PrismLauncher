#include "HashUtils.h"

#include <QDebug>
#include <QFile>

#include "FileSystem.h"

#include <MurmurHash2.h>

namespace Hashing {

static ModPlatform::ProviderCapabilities ProviderCaps;

Hasher::Ptr createHasher(QString file_path, ModPlatform::Provider provider)
{
    switch (provider) {
        case ModPlatform::Provider::MODRINTH:
            return createModrinthHasher(file_path);
        case ModPlatform::Provider::FLAME:
            return createFlameHasher(file_path);
        default:
            qCritical() << "[Hashing]"
                        << "Unrecognized mod platform!";
            return nullptr;
    }
}

Hasher::Ptr createModrinthHasher(QString file_path)
{
    return new ModrinthHasher(file_path);
}

Hasher::Ptr createFlameHasher(QString file_path)
{
    return new FlameHasher(file_path);
}

void ModrinthHasher::executeTask()
{
    QFile file(m_path);

    try {
        file.open(QFile::ReadOnly);
    } catch (FS::FileSystemException& e) {
        qCritical() << QString("Failed to open JAR file in %1").arg(m_path);
        qCritical() << QString("Reason: ") << e.cause();

        emitFailed("Failed to open file for hashing.");
        return;
    }

    auto hash_type = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH).first();
    m_hash = ProviderCaps.hash(ModPlatform::Provider::MODRINTH, &file, hash_type);

    file.close();

    if (m_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}

void FlameHasher::executeTask()
{
    // CF-specific
    auto should_filter_out = [](char c) { return (c == 9 || c == 10 || c == 13 || c == 32); };

    std::ifstream file_stream(m_path.toStdString(), std::ifstream::binary);
    // TODO: This is very heavy work, but apparently QtConcurrent can't use move semantics, so we can't boop this to another thread.
    // How do we make this non-blocking then?
    m_hash = QString::number(MurmurHash2(std::move(file_stream), 4 * MiB, should_filter_out));

    if (m_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}

}  // namespace Hashing
