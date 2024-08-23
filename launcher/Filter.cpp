#include "Filter.h"

ContainsFilter::ContainsFilter(const QString& pattern) : pattern(pattern) {}
bool ContainsFilter::accepts(const QString& value)
{
    return value.contains(pattern);
}

ExactFilter::ExactFilter(const QString& pattern) : pattern(pattern) {}
bool ExactFilter::accepts(const QString& value)
{
    return value == pattern;
}

ExactIfPresentFilter::ExactIfPresentFilter(const QString& pattern) : pattern(pattern) {}
bool ExactIfPresentFilter::accepts(const QString& value)
{
    return value.isEmpty() || value == pattern;
}

RegexpFilter::RegexpFilter(const QString& regexp, bool invert) : invert(invert)
{
    pattern.setPattern(regexp);
    pattern.optimize();
}
bool RegexpFilter::accepts(const QString& value)
{
    auto match = pattern.match(value);
    bool matched = match.hasMatch();
    return invert ? (!matched) : (matched);
}

ExactListFilter::ExactListFilter(const QStringList& pattern) : m_pattern(pattern) {}
bool ExactListFilter::accepts(const QString& value)
{
    return m_pattern.isEmpty() || m_pattern.contains(value);
}