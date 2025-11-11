// SPDX-FileCopyrightText: 2023 kumquat-ir 66188216+kumquat-ir@users.noreply.github.com
//
// SPDX-License-Identifier: LGPL-3.0-only

#include "qdcss.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

static const QRegularExpression s_rulesetRe(R"([#.]?(@?\w+?)\s*\{(.*?)\})", QRegularExpression::DotMatchesEverythingOption);
static const QRegularExpression s_ruleRe(R"((\S+?)\s*:\s*(?:\"(.*?)(?<!\\)\"|'(.*?)(?<!\\)'|(\S+?))\s*(?:;|$))");

QDCSS::QDCSS(QString s)
{
    // not much error handling over here...
    // the original java code used indeces returned by the matcher for them, but QRE does not expose those
    QRegularExpressionMatchIterator ruleset_i = s_rulesetRe.globalMatch(s);
    while (ruleset_i.hasNext()) {
        QRegularExpressionMatch ruleset = ruleset_i.next();
        QString selector = ruleset.captured(1);
        QString rules = ruleset.captured(2);
        QRegularExpressionMatchIterator rule_i = s_ruleRe.globalMatch(rules);
        while (rule_i.hasNext()) {
            QRegularExpressionMatch rule = rule_i.next();
            QString property = rule.captured(1);
            QString value;
            if (!rule.captured(2).isNull()) {
                value = rule.captured(2);
            } else if (!rule.captured(3).isNull()) {
                value = rule.captured(3);
            } else {
                value = rule.captured(4);
            }
            QString key = selector + "." + property;
            if (!m_data.contains(key)) {
                m_data.insert(key, QStringList());
            }
            m_data.find(key)->append(value);
        }
    }
}

std::optional<QString>* QDCSS::get(QString key)
{
    auto found = m_data.find(key);

    if (found == m_data.end() || found->empty()) {
        return new std::optional<QString>;
    }

    return new std::optional<QString>(found->back());
}
