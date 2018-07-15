#include "Version.h"

#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Version::Version(const QString &str) : m_string(str)
{
    parse();
}

bool Version::operator<(const Version &other) const
{
    const int size = qMax(m_sections.size(), other.m_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= m_sections.size()) ? Section("0") : m_sections.at(i);
        const Section sec2 =
            (i >= other.m_sections.size()) ? Section("0") : other.m_sections.at(i);
        if (sec1 != sec2)
        {
            return sec1 < sec2;
        }
    }

    return false;
}
bool Version::operator<=(const Version &other) const
{
    return *this < other || *this == other;
}
bool Version::operator>(const Version &other) const
{
    const int size = qMax(m_sections.size(), other.m_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= m_sections.size()) ? Section("0") : m_sections.at(i);
        const Section sec2 =
            (i >= other.m_sections.size()) ? Section("0") : other.m_sections.at(i);
        if (sec1 != sec2)
        {
            return sec1 > sec2;
        }
    }

    return false;
}
bool Version::operator>=(const Version &other) const
{
    return *this > other || *this == other;
}
bool Version::operator==(const Version &other) const
{
    const int size = qMax(m_sections.size(), other.m_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= m_sections.size()) ? Section("0") : m_sections.at(i);
        const Section sec2 =
            (i >= other.m_sections.size()) ? Section("0") : other.m_sections.at(i);
        if (sec1 != sec2)
        {
            return false;
        }
    }

    return true;
}
bool Version::operator!=(const Version &other) const
{
    return !operator==(other);
}

void Version::parse()
{
    m_sections.clear();

    // FIXME: this is bad. versions can contain a lot more separators...
    QStringList parts = m_string.split('.');

    for (const auto part : parts)
    {
        m_sections.append(Section(part));
    }
}
