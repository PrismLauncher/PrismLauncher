#include "modplatform/ModIndex.h"

namespace ModPlatform{

auto ProviderCapabilities::name(Provider p) -> const char*
{
    switch(p){
    case Provider::MODRINTH:
        return "modrinth";
    case Provider::FLAME:
        return "curseforge";
    }
}
auto ProviderCapabilities::hashType(Provider p) -> QString
{
    switch(p){
    case Provider::MODRINTH:
        return "sha512";
    case Provider::FLAME:
        return "murmur2";
    }
}

} // namespace ModPlatform
