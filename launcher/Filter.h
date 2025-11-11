#pragma once

#include <QRegularExpression>
#include <QString>

using Filter = std::function<bool(const QString&)>;

namespace Filters {
inline Filter inverse(Filter filter)
{
    return [filter = std::move(filter)](const QString& src) { return !filter(src); };
}

inline Filter any(QList<Filter> filters)
{
    return [filters = std::move(filters)](const QString& src) {
        for (auto& filter : filters)
            if (filter(src))
                return true;

        return false;
    };
}

inline Filter equals(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src == pattern; };
}

inline Filter equalsAny(QStringList patterns = {})
{
    return [patterns = std::move(patterns)](const QString& src) { return patterns.isEmpty() || patterns.contains(src); };
}

inline Filter equalsOrEmpty(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src.isEmpty() || src == pattern; };
}

inline Filter contains(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src.contains(pattern); };
}

inline Filter startsWith(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src.startsWith(pattern); };
}

inline Filter regexp(QRegularExpression pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return pattern.match(src).hasMatch(); };
}
}  // namespace Filters
