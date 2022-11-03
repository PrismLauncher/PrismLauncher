#pragma once

#include <QString>

namespace StringUtils {
int naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs);
}  // namespace StringUtils
