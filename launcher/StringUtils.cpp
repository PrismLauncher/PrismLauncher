#include "StringUtils.h"

#include <cmath>
#include <QRegularExpression>

/// If you're wondering where these came from exactly, then know you're not the only one =D

/// TAKEN FROM Qt, because it doesn't expose it intelligently
static inline QChar getNextChar(const QString& s, int location)
{
    return (location < s.length()) ? s.at(location) : QChar();
}

/// TAKEN FROM Qt, because it doesn't expose it intelligently
int StringUtils::naturalCompare(const QString& s1, const QString& s2, Qt::CaseSensitivity cs)
{
    int l1 = 0, l2 = 0;
    while (l1 <= s1.count() && l2 <= s2.count()) {
        // skip spaces, tabs and 0's
        QChar c1 = getNextChar(s1, l1);
        while (c1.isSpace())
            c1 = getNextChar(s1, ++l1);

        QChar c2 = getNextChar(s2, l2);
        while (c2.isSpace())
            c2 = getNextChar(s2, ++l2);

        if (c1.isDigit() && c2.isDigit()) {
            while (c1.digitValue() == 0)
                c1 = getNextChar(s1, ++l1);
            while (c2.digitValue() == 0)
                c2 = getNextChar(s2, ++l2);

            int lookAheadLocation1 = l1;
            int lookAheadLocation2 = l2;
            int currentReturnValue = 0;
            // find the last digit, setting currentReturnValue as we go if it isn't equal
            for (QChar lookAhead1 = c1, lookAhead2 = c2; (lookAheadLocation1 <= s1.length() && lookAheadLocation2 <= s2.length());
                 lookAhead1 = getNextChar(s1, ++lookAheadLocation1), lookAhead2 = getNextChar(s2, ++lookAheadLocation2)) {
                bool is1ADigit = !lookAhead1.isNull() && lookAhead1.isDigit();
                bool is2ADigit = !lookAhead2.isNull() && lookAhead2.isDigit();
                if (!is1ADigit && !is2ADigit)
                    break;
                if (!is1ADigit)
                    return -1;
                if (!is2ADigit)
                    return 1;
                if (currentReturnValue == 0) {
                    if (lookAhead1 < lookAhead2) {
                        currentReturnValue = -1;
                    } else if (lookAhead1 > lookAhead2) {
                        currentReturnValue = 1;
                    }
                }
            }
            if (currentReturnValue != 0)
                return currentReturnValue;
        }

        if (cs == Qt::CaseInsensitive) {
            if (!c1.isLower())
                c1 = c1.toLower();
            if (!c2.isLower())
                c2 = c2.toLower();
        }

        int r = QString::localeAwareCompare(c1, c2);
        if (r < 0)
            return -1;
        if (r > 0)
            return 1;

        l1 += 1;
        l2 += 1;
    }

    // The two strings are the same (02 == 2) so fall back to the normal sort
    return QString::compare(s1, s2, cs);
}

/// Truncate a url while keeping its readability py placing the `...` in the middle of the path  
QString StringUtils::truncateUrlHumanFriendly(QUrl &url, int max_len, bool hard_limit)
{
    auto display_options = QUrl::RemoveUserInfo | QUrl::RemoveFragment | QUrl::NormalizePathSegments;
    auto str_url = url.toDisplayString(display_options);

    if (str_url.length() <= max_len)
        return str_url;

    auto url_path_parts = url.path().split('/');
    QString last_path_segment = url_path_parts.takeLast();

    if (url_path_parts.size() >= 1 && url_path_parts.first().isEmpty())
        url_path_parts.removeFirst();  // drop empty first segment (from leading / )

    if (url_path_parts.size() >= 1)
        url_path_parts.removeLast();  // drop the next to last path segment

    auto url_template = QStringLiteral("%1://%2/%3%4");

    auto url_compact = url_path_parts.isEmpty()
                           ? url_template.arg(url.scheme(), url.host(), QStringList({ "...", last_path_segment }).join('/'), url.query())
                           : url_template.arg(url.scheme(), url.host(),
                                              QStringList({ url_path_parts.join('/'), "...", last_path_segment }).join('/'), url.query());

    // remove url parts one by one if it's still too long
    while (url_compact.length() > max_len && url_path_parts.size() >= 1) {
        url_path_parts.removeLast();  // drop the next to last path segment
        url_compact = url_path_parts.isEmpty()
                          ? url_template.arg(url.scheme(), url.host(), QStringList({ "...", last_path_segment }).join('/'), url.query())
                          : url_template.arg(url.scheme(), url.host(),
                                             QStringList({ url_path_parts.join('/'), "...", last_path_segment }).join('/'), url.query());
    }

    if ((url_compact.length() >= max_len) && hard_limit) {
        // still too long, truncate normaly
        url_compact = QString(str_url);
        auto to_remove = url_compact.length() - max_len + 3;
        url_compact.remove(url_compact.length() - to_remove - 1, to_remove);
        url_compact.append("...");
    }

    return url_compact;

}

static const QStringList s_units_si  {"KB", "MB", "GB", "TB"};
static const QStringList s_units_kibi {"KiB", "MiB", "Gib", "TiB"};

QString StringUtils::humanReadableFileSize(double bytes, bool use_si, int decimal_points) {
    const QStringList units = use_si ? s_units_si : s_units_kibi;
    const int scale = use_si ? 1000 : 1024;

    int u = -1;
    double r = pow(10,  decimal_points);

    do {
        bytes /= scale;
        u++;
    } while (round(abs(bytes) * r) / r >= scale && u < units.length() - 1);

    return QString::number(bytes, 'f', 2) + " " + units[u];
}