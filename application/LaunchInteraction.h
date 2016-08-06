#pragma once
#include <QObject>
#include <BaseInstance.h>
#include <tools/BaseProfiler.h>

class InstanceWindow;
class LaunchController: public Task
{
	Q_OBJECT
public:
	virtual void executeTask();

	LaunchController(QObject * parent = nullptr);
	virtual ~LaunchController(){};

	void setInstance(InstancePtr instance)
	{
		m_instance = instance;
	}
	void setOnline(bool online)
	{
		m_online = online;
	}
	void setProfiler(BaseProfilerFactory *profiler)
	{
		m_profiler = profiler;
	}
	void setParentWidget(QWidget * widget)
	{
		m_parentWidget = widget;
	}

private:
	void login();
	void launchInstance();

private slots:
	void readyForLaunch();

private:
	BaseProfilerFactory *m_profiler = nullptr;
	bool m_online = true;
	InstancePtr m_instance;
	QWidget * m_parentWidget = nullptr;
	InstanceWindow *m_console = nullptr;
	AuthSessionPtr m_session;
	std::shared_ptr <LaunchTask> m_launcher;
};
