#pragma once

#include <QObject>

class BaseInstance;
class SettingsObject;
class MinecraftProcess;
class QProcess;

class BaseExternalTool : public QObject
{
	Q_OBJECT
public:
	explicit BaseExternalTool(BaseInstance *instance, QObject *parent = 0);
	virtual ~BaseExternalTool();

protected:
	BaseInstance *m_instance;

	qint64 pid(QProcess *process);
	QString getSave() const;
};

class BaseDetachedTool : public BaseExternalTool
{
	Q_OBJECT
public:
	explicit BaseDetachedTool(BaseInstance *instance, QObject *parent = 0);

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

	virtual void registerSettings(SettingsObject *settings) = 0;

	virtual BaseExternalTool *createTool(BaseInstance *instance, QObject *parent = 0) = 0;

	virtual bool check(QString *error) = 0;
	virtual bool check(const QString &path, QString *error) = 0;
};

class BaseDetachedToolFactory : public BaseExternalToolFactory
{
public:
	virtual BaseDetachedTool *createDetachedTool(BaseInstance *instance, QObject *parent = 0);
};
