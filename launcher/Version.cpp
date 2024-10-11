#include "Version.h"

#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>

Version::Version(QString str) : m_string(std::move(str))
{
    parse();
}

#define VERSION_OPERATOR(return_on_different)                                               \
    bool exclude_our_sections = false;                                                      \
    bool exclude_their_sections = false;                                                    \
                                                                                            \
    const auto size = qMax(m_sections.size(), other.m_sections.size());                     \
    for (int i = 0; i < size; ++i) {                                                        \
        Section sec1 = (i >= m_sections.size()) ? Section() : m_sections.at(i);             \
        Section sec2 = (i >= other.m_sections.size()) ? Section() : other.m_sections.at(i); \
                                                                                            \
        { /* Don't include appendixes in the comparison */                                  \
            if (sec1.isAppendix())                                                          \
                exclude_our_sections = true;                                                \
            if (sec2.isAppendix())                                                          \
                exclude_their_sections = true;                                              \
                                                                                            \
            if (exclude_our_sections) {                                                     \
                sec1 = Section();                                                           \
                if (sec2.m_isNull)                                                          \
                    break;                                                                  \
            }                                                                               \
                                                                                            \
            if (exclude_their_sections) {                                                   \
                sec2 = Section();                                                           \
                if (sec1.m_isNull)                                                          \
                    break;                                                                  \
            }                                                                               \
        }                                                                                   \
                                                                                            \
        if (sec1 != sec2)                                                                   \
            return return_on_different;                                                     \
    }

bool Version::operator<(const Version& other) const
{
    VERSION_OPERATOR(sec1 < sec2)

    return false;
}
bool Version::operator==(const Version& other) const
{
    VERSION_OPERATOR(false)

    return true;
}
bool Version::operator!=(const Version& other) const
{
    return !operator==(other);
}
bool Version::operator<=(const Version& other) const
{
    return *this < other || *this == other;
}
bool Version::operator>(const Version& other) const
{
    return !(*this <= other);
}
bool Version::operator>=(const Version& other) const
{
    return !(*this < other);
}

void Version::parse()
{
    m_sections.clear();
    QString currentSection;

    if (m_string.isEmpty())
        return;

    auto classChange = [&](QChar lastChar, QChar currentChar) {
        if (lastChar.isNull())
            return false;
        if (lastChar.isDigit() != currentChar.isDigit())
            return true;

        const QList<QChar> s_separators{ '.', '-', '+' };
        if (s_separators.contains(currentChar) && currentSection.at(0) != currentChar)
            return true;

        return false;
    };

    currentSection += m_string.at(0);
    for (int i = 1; i < m_string.size(); ++i) {
        const auto& current_char = m_string.at(i);
        if (classChange(m_string.at(i - 1), current_char)) {
            if (!currentSection.isEmpty())
                m_sections.append(Section(currentSection));
            currentSection = "";
        }

        currentSection += current_char;
    }

    if (!currentSection.isEmpty())
        m_sections.append(Section(currentSection));
}

/// qDebug print support for the Version class
QDebug operator<<(QDebug debug, const Version& v)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "Version{ string: " << v.toString() << ", sections: [ ";

    bool first = true;
    for (auto s : v.m_sections) {
        if (!first)
            debug.nospace() << ", ";
        debug.nospace() << s.m_fullString;
        first = false;
    }

    debug.nospace() << " ]" << " }";

    return debug;
}
