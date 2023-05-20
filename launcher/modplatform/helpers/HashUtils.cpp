#include "HashUtils.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include "FileSystem.h"
#include "StringUtils.h"

#include <MurmurHash2.h>

namespace Hashing {

static ModPlatform::ProviderCapabilities ProviderCaps;

Hasher::Ptr createHasher(QString file_path, ModPlatform::ResourceProvider provider)
{
    switch (provider) {
        case ModPlatform::ResourceProvider::MODRINTH:
            return createModrinthHasher(file_path);
        case ModPlatform::ResourceProvider::FLAME:
            return createFlameHasher(file_path);
        default:
            qCritical() << "[Hashing]"
                        << "Unrecognized mod platform!";
            return nullptr;
    }
}

Hasher::Ptr createModrinthHasher(QString file_path)
{
    return makeShared<ModrinthHasher>(file_path);
}

Hasher::Ptr createFlameHasher(QString file_path)
{
    return makeShared<FlameHasher>(file_path);
}

Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider)
{
    return makeShared<BlockedModHasher>(file_path, provider);
}

Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider, QString type)
{
    auto hasher = makeShared<BlockedModHasher>(file_path, provider);
    hasher->useHashType(type);
    return hasher;
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

    auto hash_type = ProviderCaps.hashType(ModPlatform::ResourceProvider::MODRINTH).first();
    m_hash = ProviderCaps.hash(ModPlatform::ResourceProvider::MODRINTH, &file, hash_type);

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

    std::ifstream file_stream(StringUtils::toStdString(m_path).c_str(), std::ifstream::binary);
    // TODO: This is very heavy work, but apparently QtConcurrent can't use move semantics, so we can't boop this to another thread.
    // How do we make this non-blocking then?
    m_hash = QString::number(MurmurHash2(std::move(file_stream), 4 * MiB, should_filter_out));

    if (m_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}


BlockedModHasher::BlockedModHasher(QString file_path, ModPlatform::ResourceProvider provider)
    : Hasher(file_path), provider(provider) { 
    setObjectName(QString("BlockedModHasher: %1").arg(file_path)); 
    hash_type = ProviderCaps.hashType(provider).first();
}

void BlockedModHasher::executeTask()
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

    m_hash = ProviderCaps.hash(provider, &file, hash_type);

    file.close();

    if (m_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}

QStringList BlockedModHasher::getHashTypes() {
    return ProviderCaps.hashType(provider);
}

bool BlockedModHasher::useHashType(QString type) {
    auto types = ProviderCaps.hashType(provider);
    if (types.contains(type)) {
        hash_type = type;
        return true;
    }
    qDebug() << "Bad hash type " << type << " for provider";
    return false;
}

const static QMap<HashType, QString> s_hash_type_names = {
    { HashType::Md5, "md5" },
    { HashType::Sha1, "sha1" },
    { HashType::Sha256, "sha256" },
    { HashType::Sha256, "sha256" },
};

HashType guessHashType(const QString& hash)
{
    QRegularExpression hexMatcher("^[0-9A-Fa-f]{32,128}$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = hexMatcher.match(hash);
    if (!match.hasMatch()) {
        // did not found a hex string with a length between 32 and 128
        return HashType::UNKNOWN;
    }
    auto len = hash.length();
    switch (len) {
        case 128 / 8 * 2:  // MD5 128 bits -> hex
            return HashType::Md5;
        case 160 / 8 * 2:  // SHA1 160 bits -> hex
            return HashType::Sha1;
        case 256 / 8 * 2:  // SHA256 256 bits -> hex
            return HashType::Sha256;
        case 512 / 8 * 2:  // SHA512 512 bis -> hex
            return HashType::Sha512;
        default:
            return HashType::UNKNOWN;
    }
}
QString getHashTypeName(HashType type)
{
    return s_hash_type_names.value(type, "unknown");
}

HashType getHashTypeFromName(const QString& name)
{
    return s_hash_type_names.key(name, HashType::UNKNOWN);
}

}  // namespace Hashing
