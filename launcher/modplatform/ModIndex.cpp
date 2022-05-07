#include "modplatform/ModIndex.h"

#include <QCryptographicHash>

namespace ModPlatform {

auto ProviderCapabilities::name(Provider p) -> const char*
{
    switch (p) {
        case Provider::MODRINTH:
            return "modrinth";
        case Provider::FLAME:
            return "curseforge";
    }
    return {};
}
auto ProviderCapabilities::readableName(Provider p) -> QString
{
    switch (p) {
        case Provider::MODRINTH:
            return "Modrinth";
        case Provider::FLAME:
            return "CurseForge";
    }
    return {};
}
auto ProviderCapabilities::hashType(Provider p) -> QStringList
{
    switch (p) {
        case Provider::MODRINTH:
            return { "sha512", "sha1" };
        case Provider::FLAME:
            // Try newer formats first, fall back to old format
            return { "sha1", "md5", "murmur2" };
    }
    return {};
}
auto ProviderCapabilities::hash(Provider p, QByteArray& data, QString type) -> QByteArray
{
    switch (p) {
        case Provider::MODRINTH: {
            // NOTE: Data is the result of reading the entire JAR file!

            // If 'type' was specified, we use that
            if (!type.isEmpty() && hashType(p).contains(type)) {
                if (type == "sha512")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
                else if (type == "sha1")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
            }

            return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
        }
        case Provider::FLAME:
            // If 'type' was specified, we use that
            if (!type.isEmpty() && hashType(p).contains(type)) {
                if(type == "sha1")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
                else if (type == "md5")
                    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
            }
            
            break;
    }
    return {};
}

}  // namespace ModPlatform
