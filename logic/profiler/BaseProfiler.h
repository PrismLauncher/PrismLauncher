#pragma once

#include <QObject>

class BaseInstance;
class SettingsObject;
class MinecraftProcess;
class QProcess;

class BaseProfiler : public QObject
{
	Q_OBJECT
public:
	explicit BaseProfiler(BaseInstance *instance, QObject *parent = 0);
	virtual ~BaseProfiler();

public
slots:
	void beginProfiling(MinecraftProcess *process);
	void abortProfiling();

protected:
	BaseInstance *m_instance;
	QProcess *m_profilerProcess;

	virtual void beginProfilingImpl(MinecraftProcess *process) = 0;
	virtual void abortProfilingImpl();

	qint64 pid(QProcess *process);

signals:
	void readyToLaunch(const QString &message);
	void abortLaunch(const QString &message);
};

class BaseProfilerFactory
{
public:
	virtual ~BaseProfilerFactory();

	virtual QString name() const = 0;

	virtual void registerSettings(SettingsObject *settings) = 0;

	virtual BaseProfiler *createProfiler(BaseInstance *instance, QObject *parent = 0) = 0;

	virtual bool check(QString *error) = 0;
	virtual bool check(const QString &path, QString *error) = 0;
};
