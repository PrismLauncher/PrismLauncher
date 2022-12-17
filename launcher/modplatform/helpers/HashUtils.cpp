#include "HashUtils.h"

#include <QDebug>
#include <QFile>

#include "FileSystem.h"
#include "StringUtils.h"

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

Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::Provider provider)
{
    return new BlockedModHasher(file_path, provider);
}

Hasher::Ptr createBlockedModHasher(QString file_path, ModPlatform::Provider provider, QString type)
{
    auto hasher = new BlockedModHasher(file_path, provider);
    hasher->useHashType(type);
    return hasher;
}

void ModrinthHasher::executeTask()
{
    QFile file(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path);

    try {
        file.open(QFile::ReadOnly);
    } catch (FS::FileSystemException& e) {
        qCritical() << QString("Failed to open JAR file in %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path);
        qCritical() << QString("Reason: ") << e.cause();

        emitFailed("Failed to open file for hashing.");
        return;
    }

    auto hash_type = ProviderCaps.hashType(ModPlatform::Provider::MODRINTH).first();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash = ProviderCaps.hash(ModPlatform::Provider::MODRINTH, &file, hash_type);

    file.close();

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}

void FlameHasher::executeTask()
{
    // CF-specific
    auto should_filter_out = [](char c) { return (c == 9 || c == 10 || c == 13 || c == 32); };

    std::ifstream file_stream(StringUtils::toStdString(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path), std::ifstream::binary);
    // TODO: This is very heavy work, but apparently QtConcurrent can't use move semantics, so we can't boop this to another thread.
    // How do we make this non-blocking then?
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash = QString::number(MurmurHash2(std::move(file_stream), 4 * MiB, should_filter_out));

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash.isEmpty()) {
        emitFailed("Empty hash!");
    } else {
        emitSucceeded();
    }
}


BlockedModHasher::BlockedModHasher(QString file_path, ModPlatform::Provider provider) 
    : Hasher(file_path), provider(provider) { 
    setObjectName(QString("BlockedModHasher: %1").arg(file_path)); 
    hash_type = ProviderCaps.hashType(provider).first();
}

void BlockedModHasher::executeTask()
{
    QFile file(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path);

    try {
        file.open(QFile::ReadOnly);
    } catch (FS::FileSystemException& e) {
        qCritical() << QString("Failed to open JAR file in %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path);
        qCritical() << QString("Reason: ") << e.cause();

        emitFailed("Failed to open file for hashing.");
        return;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash = ProviderCaps.hash(provider, &file, hash_type);

    file.close();

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hash.isEmpty()) {
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

}  // namespace Hashing
