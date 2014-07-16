#pragma once

#include <QString>
#include <QStringList>
#include "logic/DefaultVariable.h"

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
		groupId = elements[1];
		artifactId = elements[2];
		version = elements[3];
		classifier = elements[5];
		if(!elements[7].isEmpty())
		{
			extension = elements[7];
		}
		return *this;
	}
	operator QString() const
	{
		if(!m_valid)
			return "INVALID";
		QString retval = groupId + ":" + artifactId + ":" + version;
		if(!classifier.isEmpty())
		{
			retval += ":" + classifier;
		}
		if(extension.isExplicit())
		{
			retval += "@" + extension;
		}
		return retval;
	}
	QString toPath() const
	{
		if(!m_valid)
			return "INVALID";
		QString path = groupId;
		path.replace('.', '/');
		path += '/' + artifactId + '/' + version + '/' + artifactId + '-' + version;
		if(!classifier.isEmpty())
		{
			path += "-" + classifier;
		}
		path += "." + extension;
		return path;
	}
	bool valid()
	{
		return m_valid;
	}
private:
	QString groupId;
	QString artifactId;
	QString version;
	QString classifier;
	DefaultVariable<QString> extension = DefaultVariable<QString>("jar");
	bool m_valid = false;
};
