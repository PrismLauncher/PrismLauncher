#pragma once

#include <QString>

namespace StringUtils {

#if defined Q_OS_WIN32
using string = std::wstring;

inline string toStdString(QString s)
{
    return s.toStdWString();
}
inline QString fromStdString(string s)
{
    return QString::fromStdWString(s);
}
#else
using string = std::string;

inline string toStdString(QString s)
{
    return s.toStdString();
}
inline QString fromStdString(string s)
{
    return QString::fromStdString(s);
}
#endif

int naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs);
}  // namespace StringUtils
