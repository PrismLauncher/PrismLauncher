// SPDX-FileCopyrightText: 2023 kumquat-ir 66188216+kumquat-ir@users.noreply.github.com
//
// SPDX-License-Identifier: LGPL-3.0-only

#include "qdcss.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

QRegularExpression ruleset_re = QRegularExpression(R"([#.]?(@?\w+?)\s*\{(.*?)\})", QRegularExpression::DotMatchesEverythingOption);
QRegularExpression rule_re = QRegularExpression(R"((\S+?)\s*:\s*(?:\"(.*?)(?<!\\)\"|'(.*?)(?<!\\)'|(\S+?))\s*(?:;|$))");

QDCSS::QDCSS(QString s)
{
    // not much error handling over here...
    // the original java code used indeces returned by the matcher for them, but QRE does not expose those
    QRegularExpressionMatchIterator ruleset_i = ruleset_re.globalMatch(s);
    while (ruleset_i.hasNext()) {
        QRegularExpressionMatch ruleset = ruleset_i.next();
        QString selector = ruleset.captured(1);
        QString rules = ruleset.captured(2);
        QRegularExpressionMatchIterator rule_i = rule_re.globalMatch(rules);
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
