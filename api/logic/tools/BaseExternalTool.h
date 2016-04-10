#pragma once

#include <QObject>
#include <BaseInstance.h>

#include "multimc_logic_export.h"

class BaseInstance;
class SettingsObject;
class QProcess;

class MULTIMC_LOGIC_EXPORT BaseExternalTool : public QObject
{
	Q_OBJECT
public:
	explicit BaseExternalTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);
	virtual ~BaseExternalTool();

protected:
	InstancePtr m_instance;
	SettingsObjectPtr globalSettings;
};

class MULTIMC_LOGIC_EXPORT BaseDetachedTool : public BaseExternalTool
{
	Q_OBJECT
public:
	explicit BaseDetachedTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

public
slots:
	void run();

protected:
	virtual void runImpl() = 0;
};

class MULTIMC_LOGIC_EXPORT BaseExternalToolFactory
{
public:
	virtual ~BaseExternalToolFactory();

	virtual QString name() const = 0;

	virtual void registerSettings(SettingsObjectPtr settings) = 0;

	virtual BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) = 0;

	virtual bool check(QString *error) = 0;
	virtual bool check(const QString &path, QString *error) = 0;

protected:
	SettingsObjectPtr globalSettings;
};

class MULTIMC_LOGIC_EXPORT BaseDetachedToolFactory : public BaseExternalToolFactory
{
public:
	virtual BaseDetachedTool *createDetachedTool(InstancePtr instance, QObject *parent = 0);
};
