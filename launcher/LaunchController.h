#pragma once
#include <QObject>
#include <BaseInstance.h>
#include <tools/BaseProfiler.h>

#include "minecraft/launch/MinecraftServerTarget.h"

class InstanceWindow;
class LaunchController: public Task
{
    Q_OBJECT
public:
    void executeTask() override;

    LaunchController(QObject * parent = nullptr);
    virtual ~LaunchController(){};

    void setInstance(InstancePtr instance)
    {
        m_instance = instance;
    }
    InstancePtr instance()
    {
        return m_instance;
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
    void setServerToJoin(MinecraftServerTargetPtr serverToJoin)
    {
        m_serverToJoin = std::move(serverToJoin);
    }
    QString id()
    {
        return m_instance->id();
    }
    bool abort() override;

private:
    void login();
    void launchInstance();

private slots:
    void readyForLaunch();

    void onSucceeded();
    void onFailed(QString reason);
    void onProgressRequested(Task *task);

private:
    BaseProfilerFactory *m_profiler = nullptr;
    bool m_online = true;
    InstancePtr m_instance;
    QWidget * m_parentWidget = nullptr;
    InstanceWindow *m_console = nullptr;
    AuthSessionPtr m_session;
    shared_qobject_ptr<LaunchTask> m_launcher;
    MinecraftServerTargetPtr m_serverToJoin;
};
