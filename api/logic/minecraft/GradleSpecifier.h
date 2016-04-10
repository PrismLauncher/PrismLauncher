#pragma once

#include <QString>
#include <QStringList>
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
		DEBUG   0 "org.gradle.test.classifiers:service:1.0:jdk15@jar" 
		DEBUG   1 "org.gradle.test.classifiers" 
		DEBUG   2 "service" 
		DEBUG   3 "1.0" 
		DEBUG   4 ":jdk15" 
		DEBUG   5 "jdk15" 
		DEBUG   6 "@jar" 
		DEBUG   7 "jar"
		*/
		QRegExp matcher("([^:@]+):([^:@]+):([^:@]+)" "(:([^:@]+))?" "(@([^:@]+))?");
		m_valid = matcher.exactMatch(value);
		auto elements = matcher.capturedTexts();
		m_groupId = elements[1];
		m_artifactId = elements[2];
		m_version = elements[3];
		m_classifier = elements[5];
		if(!elements[7].isEmpty())
		{
			m_extension = elements[7];
		}
		return *this;
	}
	operator QString() const
	{
		if(!m_valid)
			return "INVALID";
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
	QString toPath() const
	{
		if(!m_valid)
			return "INVALID";
		QString path = m_groupId;
		path.replace('.', '/');
		path += '/' + m_artifactId + '/' + m_version + '/' + m_artifactId + '-' + m_version;
		if(!m_classifier.isEmpty())
		{
			path += "-" + m_classifier;
		}
		path += "." + m_extension;
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
		return other.artifactId() == artifactId() && other.groupId() == groupId();
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
	QString m_groupId;
	QString m_artifactId;
	QString m_version;
	QString m_classifier;
	DefaultVariable<QString> m_extension = DefaultVariable<QString>("jar");
	bool m_valid = false;
};
