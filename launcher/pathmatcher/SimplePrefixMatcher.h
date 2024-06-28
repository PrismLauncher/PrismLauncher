// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <QRegularExpression>
#include "IPathMatcher.h"

class SimplePrefixMatcher : public IPathMatcher {
   public:
    virtual ~SimplePrefixMatcher() {};
    SimplePrefixMatcher(const QString& prefix)
    {
        m_prefix = prefix;
        m_isPrefix = prefix.endsWith('/');
    }

    virtual bool matches(const QString& string) const override
    {
        if (m_isPrefix)
            return string.startsWith(m_prefix);
        return string == m_prefix;
    }
    QString m_prefix;
    bool m_isPrefix = false;
};
