#pragma once

#include <QString>

namespace StringUtils {

#if defined Q_OS_WIN32
inline std::wstring toStdString(QString s)
{
    return s.toStdWString();
}
#else
inline std::string toStdString(QString s)
{
    return s.toStdString();
}
#endif

int naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs);
}  // namespace StringUtils
