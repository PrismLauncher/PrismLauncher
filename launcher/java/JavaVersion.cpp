#include "JavaVersion.h"

#include "StringUtils.h"

#include <QRegularExpression>
#include <QString>

JavaVersion& JavaVersion::operator=(const QString& javaVersionString)
{
    m_string = javaVersionString;

    auto getCapturedInteger = [](const QRegularExpressionMatch& match, const QString& what) -> int {
        auto str = match.captured(what);
        if (str.isEmpty()) {
            return 0;
        }
        return str.toInt();
    };

    QRegularExpression pattern;
    if (javaVersionString.startsWith("1.")) {
        pattern = QRegularExpression("1[.](?<major>[0-9]+)([.](?<minor>[0-9]+))?(_(?<security>[0-9]+)?)?(-(?<prerelease>[a-zA-Z0-9]+))?");
    } else {
        pattern = QRegularExpression("(?<major>[0-9]+)([.](?<minor>[0-9]+))?([.](?<security>[0-9]+))?(-(?<prerelease>[a-zA-Z0-9]+))?");
    }

    auto match = pattern.match(m_string);
    m_parseable = match.hasMatch();
    m_major = getCapturedInteger(match, "major");
    m_minor = getCapturedInteger(match, "minor");
    m_security = getCapturedInteger(match, "security");
    m_prerelease = match.captured("prerelease");
    return *this;
}

JavaVersion::JavaVersion(const QString& rhs)
{
    operator=(rhs);
}

QString JavaVersion::toString() const
{
    return m_string;
}

bool JavaVersion::requiresPermGen() const
{
    return !m_parseable || m_major < 8;
}

bool JavaVersion::defaultsToUtf8() const
{
    // starting from Java 18, UTF-8 is the default charset: https://openjdk.org/jeps/400
    return m_parseable && m_major >= 18;
}

bool JavaVersion::isModular() const
{
    return m_parseable && m_major >= 9;
}

bool JavaVersion::operator<(const JavaVersion& rhs)
{
    if (m_parseable && rhs.m_parseable) {
        auto major = m_major;
        auto rmajor = rhs.m_major;

        if (major < rmajor)
            return true;
        if (major > rmajor)
            return false;
        if (m_minor < rhs.m_minor)
            return true;
        if (m_minor > rhs.m_minor)
            return false;
        if (m_security < rhs.m_security)
            return true;
        if (m_security > rhs.m_security)
            return false;

        // everything else being equal, consider prerelease status
        bool thisPre = !m_prerelease.isEmpty();
        bool rhsPre = !rhs.m_prerelease.isEmpty();
        if (thisPre && !rhsPre) {
            // this is a prerelease and the other one isn't -> lesser
            return true;
        } else if (!thisPre && rhsPre) {
            // this isn't a prerelease and the other one is -> greater
            return false;
        } else if (thisPre && rhsPre) {
            // both are prereleases - use natural compare...
            return StringUtils::naturalCompare(m_prerelease, rhs.m_prerelease, Qt::CaseSensitive) < 0;
        }
        // neither is prerelease, so they are the same -> this cannot be less than rhs
        return false;
    } else
        return StringUtils::naturalCompare(m_string, rhs.m_string, Qt::CaseSensitive) < 0;
}

bool JavaVersion::operator==(const JavaVersion& rhs)
{
    if (m_parseable && rhs.m_parseable) {
        return m_major == rhs.m_major && m_minor == rhs.m_minor && m_security == rhs.m_security && m_prerelease == rhs.m_prerelease;
    }
    return m_string == rhs.m_string;
}

bool JavaVersion::operator>(const JavaVersion& rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}

JavaVersion::JavaVersion(int major, int minor, int security, int build, QString name)
    : m_major(major), m_minor(minor), m_security(security), m_name(name), m_parseable(true)
{
    QStringList versions;
    if (build != 0) {
        m_prerelease = QString::number(build);
        versions.push_front(m_prerelease);
    }
    if (m_security != 0)
        versions.push_front(QString::number(m_security));
    else if (!versions.isEmpty())
        versions.push_front("0");

    if (m_minor != 0)
        versions.push_front(QString::number(m_minor));
    else if (!versions.isEmpty())
        versions.push_front("0");
    versions.push_front(QString::number(m_major));
    m_string = versions.join(".");
}
