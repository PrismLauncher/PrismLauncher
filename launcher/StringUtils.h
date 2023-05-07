#pragma once

#include <QString>
#include <QUrl>

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

/**
 * @brief Truncate a url while keeping its readability py placing the `...` in the middle of the path  
 * @param url Url to truncate
 * @param max_len max lenght of url in charaters
 * @param hard_limit if truncating the path can't get the url short enough, truncate it normaly.
 */
QString truncateUrlHumanFriendly(QUrl &url, int max_len, bool hard_limit = false);

QString humanReadableFileSize(double bytes, bool use_si = false, int decimal_points = 1);

}  // namespace StringUtils
