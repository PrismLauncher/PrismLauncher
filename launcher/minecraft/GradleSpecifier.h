#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include "DefaultVariable.h"

struct GradleSpecifier
{
    GradleSpecifier()
    {
        m_valid = false;
    }
    GradleSpecifier(QString value)
    {
        operator=(value);
    }
    GradleSpecifier & operator =(const QString & value)
    {
        /*
        org.gradle.test.classifiers : service : 1.0 : jdk15 @ jar
         0 "org.gradle.test.classifiers:service:1.0:jdk15@jar"
         1 "org.gradle.test.classifiers"
         2 "service"
         3 "1.0"
         4 "jdk15"
         5 "jar"
        */
        QRegularExpression matcher(QRegularExpression::anchoredPattern("([^:@]+):([^:@]+):([^:@]+)" "(?::([^:@]+))?" "(?:@([^:@]+))?"));
        QRegularExpressionMatch match = matcher.match(value);
        m_valid = match.hasMatch();
        if(!m_valid) {
            m_invalidValue = value;
            return *this;
        }
        auto elements = match.captured();
        m_groupId = match.captured(1);
        m_artifactId = match.captured(2);
        m_version = match.captured(3);
        m_classifier = match.captured(4);
        if(match.lastCapturedIndex() >= 5)
        {
            m_extension = match.captured(5);
        }
        return *this;
    }
    QString serialize() const
    {
        if(!m_valid) {
            return m_invalidValue;
        }
        QString retval = m_groupId + ":" + m_artifactId + ":" + m_version;
        if(!m_classifier.isEmpty())
        {
            retval += ":" + m_classifier;
        }
        if(m_extension.isExplicit())
        {
            retval += "@" + m_extension;
        }
        return retval;
    }
    QString getFileName() const
    {
        if(!m_valid) {
            return QString();
        }
        QString filename = m_artifactId + '-' + m_version;
        if(!m_classifier.isEmpty())
        {
            filename += "-" + m_classifier;
        }
        filename += "." + m_extension;
        return filename;
    }
    QString toPath(const QString & filenameOverride = QString()) const
    {
        if(!m_valid) {
            return QString();
        }
        QString filename;
        if(filenameOverride.isEmpty())
        {
            filename = getFileName();
        }
        else
        {
            filename = filenameOverride;
        }
        QString path = m_groupId;
        path.replace('.', '/');
        path += '/' + m_artifactId + '/' + m_version + '/' + filename;
        return path;
    }
    inline bool valid() const
    {
        return m_valid;
    }
    inline QString version() const
    {
        return m_version;
    }
    inline QString groupId() const
    {
        return m_groupId;
    }
    inline QString artifactId() const
    {
        return m_artifactId;
    }
    inline void setClassifier(const QString & classifier)
    {
        m_classifier = classifier;
    }
    inline QString classifier() const
    {
        return m_classifier;
    }
    inline QString extension() const
    {
        return m_extension;
    }
    inline QString artifactPrefix() const
    {
        return m_groupId + ":" + m_artifactId;
    }
    bool matchName(const GradleSpecifier & other) const
    {
        return other.artifactId() == artifactId() && other.groupId() == groupId() && other.classifier() == classifier();
    }
    bool operator==(const GradleSpecifier & other) const
    {
        if(m_groupId != other.m_groupId)
            return false;
        if(m_artifactId != other.m_artifactId)
            return false;
        if(m_version != other.m_version)
            return false;
        if(m_classifier != other.m_classifier)
            return false;
        if(m_extension != other.m_extension)
            return false;
        return true;
    }
private:
    QString m_invalidValue;
    QString m_groupId;
    QString m_artifactId;
    QString m_version;
    QString m_classifier;
    DefaultVariable<QString> m_extension = DefaultVariable<QString>("jar");
    bool m_valid = false;
};
