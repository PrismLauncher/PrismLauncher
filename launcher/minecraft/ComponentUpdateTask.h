#pragma once

#include "net/Mode.h"
#include "tasks/Task.h"

#include <memory>
class PackProfile;
struct ComponentUpdateTaskData;

class ComponentUpdateTask : public Task {
    Q_OBJECT
   public:
    enum class Mode { Launch, Resolution };

   public:
    explicit ComponentUpdateTask(Mode mode, Net::Mode netmode, PackProfile* list, QObject* parent = 0);
    virtual ~ComponentUpdateTask();

   protected:
    void executeTask();

   private:
    void loadComponents();
    void resolveDependencies(bool checkOnly);

    void remoteLoadSucceeded(size_t index);
    void remoteLoadFailed(size_t index, const QString& msg);
    void checkIfAllFinished();

   private:
    std::unique_ptr<ComponentUpdateTaskData> d;
};
