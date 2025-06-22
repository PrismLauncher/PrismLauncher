#pragma once

#include "minecraft/Component.h"
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
    explicit ComponentUpdateTask(Mode mode, Net::Mode netmode, PackProfile* list);
    virtual ~ComponentUpdateTask();

   protected:
    void executeTask();

   private:
    void loadComponents();
    /// collects components that are dependent on or dependencies of the component
    QList<ComponentPtr> collectTreeLinked(const QString& uid);
    void resolveDependencies(bool checkOnly);
    void performUpdateActions();
    void finalizeComponents();

    void remoteLoadSucceeded(size_t index);
    void remoteLoadFailed(size_t index, const QString& msg);
    void checkIfAllFinished();

   private:
    std::unique_ptr<ComponentUpdateTaskData> d;
};
