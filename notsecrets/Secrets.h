#pragma once
#include <QString>
#include <cstdint>

namespace Secrets {
bool hasMSAClientID();
QString getMSAClientID(uint8_t separator);
}
