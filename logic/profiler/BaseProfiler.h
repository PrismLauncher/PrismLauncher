#pragma once

#include <QObject>

class OneSixInstance;
class SettingsObject;
class MinecraftProcess;

class BaseProfiler : public QObject
{
	Q_OBJECT
public:
	explicit BaseProfiler(OneSixInstance *instance, QObject *parent = 0);
	virtual ~BaseProfiler();

public
slots:
	void beginProfiling(MinecraftProcess *process);

protected:
	OneSixInstance *m_instance;

	virtual void beginProfilingImpl(MinecraftProcess *process) = 0;

signals:
	void readyToLaunch(const QString &message);
};

class BaseProfilerFactory
{
public:
	virtual ~BaseProfilerFactory();

	virtual void registerSettings(SettingsObject *settings) = 0;

	virtual BaseProfiler *createProfiler(OneSixInstance *instance, QObject *parent = 0) = 0;

	virtual bool check(const QString &path, QString *error) = 0;
};
