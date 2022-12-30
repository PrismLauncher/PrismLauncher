#pragma once

#include "tasks/Task.h"
#include "net/Mode.h"

#include <memory>
class PackProfile;
struct ComponentUpdateTaskData;

class ComponentUpdateTask : public Task
{
    Q_OBJECT
public:
    enum class Mode
    {
        Launch,
        Resolution
    };

public:
    explicit ComponentUpdateTask(Mode mode, Net::Mode netmode, PackProfile * list, QObject *parent = 0);
    virtual ~ComponentUpdateTask();

protected:
    void executeTask();

private:
    void loadComponents(bool firstRun);
    void resolveDependencies(bool checkOnly, bool firstRun);

    void remoteLoadSucceeded(size_t index, bool firstRun);
    void remoteLoadFailed(size_t index, const QString &msg, bool firstRun);
    void checkIfAllFinished(bool firstRun);

private:
    std::unique_ptr<ComponentUpdateTaskData> d;
};
