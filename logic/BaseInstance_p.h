#pragma once
#include <QString>
#include <settingsobject.h>

class BaseInstance;

#define I_D(Class) Class##Private * const d = (Class##Private * const) inst_d.data()

struct BaseInstancePrivate
{
	QString m_rootDir;
	QString m_group;
	SettingsObject *m_settings;
};