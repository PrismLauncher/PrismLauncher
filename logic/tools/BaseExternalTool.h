#pragma once

#include <QObject>
#include <logic/BaseInstance.h>

class BaseInstance;
class SettingsObject;
class QProcess;

class BaseExternalTool : public QObject
{
	Q_OBJECT
public:
	explicit BaseExternalTool(InstancePtr instance, QObject *parent = 0);
	virtual ~BaseExternalTool();

protected:
	InstancePtr m_instance;

	qint64 pid(QProcess *process);
};

class BaseDetachedTool : public BaseExternalTool
{
	Q_OBJECT
public:
	explicit BaseDetachedTool(InstancePtr instance, QObject *parent = 0);

public
slots:
	void run();

protected:
	virtual void runImpl() = 0;
};

class BaseExternalToolFactory
{
public:
	virtual ~BaseExternalToolFactory();

	virtual QString name() const = 0;

	virtual void registerSettings(std::shared_ptr<SettingsObject> settings) = 0;

	virtual BaseExternalTool *createTool(InstancePtr instance, QObject *parent = 0) = 0;

	virtual bool check(QString *error) = 0;
	virtual bool check(const QString &path, QString *error) = 0;
};

class BaseDetachedToolFactory : public BaseExternalToolFactory
{
public:
	virtual BaseDetachedTool *createDetachedTool(InstancePtr instance, QObject *parent = 0);
};
