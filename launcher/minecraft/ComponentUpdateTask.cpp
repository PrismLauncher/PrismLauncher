#include "ComponentUpdateTask.h"

#include "Component.h"
#include "ComponentUpdateTask_p.h"
#include "PackProfile.h"
#include "PackProfile_p.h"
#include "Version.h"
#include "cassert"
#include "meta/Index.h"
#include "meta/Version.h"
#include "minecraft/OneSixVersionFormat.h"
#include "minecraft/ProfileUtils.h"
#include "net/Mode.h"

#include "Application.h"

/*
 * This is responsible for loading the components of a component list AND resolving dependency issues between them
 */

/*
 * FIXME: the 'one shot async task' nature of this does not fit the intended usage
 * Really, it should be a reactor/state machine that receives input from the application
 * and dynamically adapts to changing requirements...
 *
 * The reactor should be the only entry into manipulating the PackProfile.
 * See: https://en.wikipedia.org/wiki/Reactor_pattern
 */

/*
 * Or make this operate on a snapshot of the PackProfile state, then merge results in as long as the snapshot and PackProfile didn't change?
 * If the component list changes, start over.
 */

ComponentUpdateTask::ComponentUpdateTask(Mode mode, Net::Mode netmode, PackProfile* list, QObject* parent) : Task(parent)
{
    d.reset(new ComponentUpdateTaskData);
    d->m_list = list;
    d->mode = mode;
    d->netmode = netmode;
}

ComponentUpdateTask::~ComponentUpdateTask() {}

void ComponentUpdateTask::executeTask()
{
    qDebug() << "Loading components";
    loadComponents();
}

namespace {
enum class LoadResult { LoadedLocal, RequiresRemote, Failed };

LoadResult composeLoadResult(LoadResult a, LoadResult b)
{
    if (a < b) {
        return b;
    }
    return a;
}

static LoadResult loadComponent(ComponentPtr component, Task::Ptr& loadTask, Net::Mode netmode)
{
    if (component->m_loaded) {
        qDebug() << component->getName() << "is already loaded";
        return LoadResult::LoadedLocal;
    }

    LoadResult result = LoadResult::Failed;
    auto customPatchFilename = component->getFilename();
    if (QFile::exists(customPatchFilename)) {
        // if local file exists...

        // check for uid problems inside...
        bool fileChanged = false;
        auto file = ProfileUtils::parseJsonFile(QFileInfo(customPatchFilename), false);
        if (file->uid != component->m_uid) {
            file->uid = component->m_uid;
            fileChanged = true;
        }
        if (fileChanged) {
            // FIXME: @QUALITY do not ignore return value
            ProfileUtils::saveJsonFile(OneSixVersionFormat::versionFileToJson(file), customPatchFilename);
        }

        component->m_file = file;
        component->m_loaded = true;
        result = LoadResult::LoadedLocal;
    } else {
        auto metaVersion = APPLICATION->metadataIndex()->get(component->m_uid, component->m_version);
        component->m_metaVersion = metaVersion;
        if (metaVersion->isLoaded()) {
            component->m_loaded = true;
            result = LoadResult::LoadedLocal;
        } else {
            metaVersion->load(netmode);
            loadTask = metaVersion->getCurrentTask();
            if (loadTask)
                result = LoadResult::RequiresRemote;
            else if (metaVersion->isLoaded())
                result = LoadResult::LoadedLocal;
            else
                result = LoadResult::Failed;
        }
    }
    return result;
}

// FIXME: dead code. determine if this can still be useful?
/*
static LoadResult loadPackProfile(ComponentPtr component, Task::Ptr& loadTask, Net::Mode netmode)
{
    if(component->m_loaded)
    {
        qDebug() << component->getName() << "is already loaded";
        return LoadResult::LoadedLocal;
    }

    LoadResult result = LoadResult::Failed;
    auto metaList = APPLICATION->metadataIndex()->get(component->m_uid);
    if(metaList->isLoaded())
    {
        component->m_loaded = true;
        result = LoadResult::LoadedLocal;
    }
    else
    {
        metaList->load(netmode);
        loadTask = metaList->getCurrentTask();
        result = LoadResult::RequiresRemote;
    }
    return result;
}
*/

static LoadResult loadIndex(Task::Ptr& loadTask, Net::Mode netmode)
{
    // FIXME: DECIDE. do we want to run the update task anyway?
    if (APPLICATION->metadataIndex()->isLoaded()) {
        qDebug() << "Index is already loaded";
        return LoadResult::LoadedLocal;
    }
    APPLICATION->metadataIndex()->load(netmode);
    loadTask = APPLICATION->metadataIndex()->getCurrentTask();
    if (loadTask) {
        return LoadResult::RequiresRemote;
    }
    // FIXME: this is assuming the load succeeded... did it really?
    return LoadResult::LoadedLocal;
}
}  // namespace

void ComponentUpdateTask::loadComponents()
{
    LoadResult result = LoadResult::LoadedLocal;
    size_t taskIndex = 0;
    size_t componentIndex = 0;
    d->remoteLoadSuccessful = true;
    // load the main index (it is needed to determine if components can revert)
    {
        // FIXME: tear out as a method? or lambda?
        Task::Ptr indexLoadTask;
        auto singleResult = loadIndex(indexLoadTask, d->netmode);
        result = composeLoadResult(result, singleResult);
        if (indexLoadTask) {
            qDebug() << "Remote loading is being run for metadata index";
            RemoteLoadStatus status;
            status.type = RemoteLoadStatus::Type::Index;
            d->remoteLoadStatusList.append(status);
            connect(indexLoadTask.get(), &Task::succeeded, [=]() { remoteLoadSucceeded(taskIndex); });
            connect(indexLoadTask.get(), &Task::failed, [=](const QString& error) { remoteLoadFailed(taskIndex, error); });
            connect(indexLoadTask.get(), &Task::aborted, [=]() { remoteLoadFailed(taskIndex, tr("Aborted")); });
            taskIndex++;
        }
    }
    // load all the components OR their lists...
    for (auto component : d->m_list->d->components) {
        Task::Ptr loadTask;
        LoadResult singleResult;
        RemoteLoadStatus::Type loadType;
        // FIXME: to do this right, we need to load the lists and decide on which versions to use during dependency resolution. For now,
        // ignore all that...
#if 0
        switch(d->mode)
        {
            case Mode::Launch:
            {
                singleResult = loadComponent(component, loadTask, d->netmode);
                loadType = RemoteLoadStatus::Type::Version;
                break;
            }
            case Mode::Resolution:
            {
                singleResult = loadPackProfile(component, loadTask, d->netmode);
                loadType = RemoteLoadStatus::Type::List;
                break;
            }
        }
#else
        singleResult = loadComponent(component, loadTask, d->netmode);
        loadType = RemoteLoadStatus::Type::Version;
#endif
        if (singleResult == LoadResult::LoadedLocal) {
            component->updateCachedData();
        }
        result = composeLoadResult(result, singleResult);
        if (loadTask) {
            qDebug() << "Remote loading is being run for" << component->getName();
            connect(loadTask.get(), &Task::succeeded, [=]() { remoteLoadSucceeded(taskIndex); });
            connect(loadTask.get(), &Task::failed, [=](const QString& error) { remoteLoadFailed(taskIndex, error); });
            connect(loadTask.get(), &Task::aborted, [=]() { remoteLoadFailed(taskIndex, tr("Aborted")); });
            RemoteLoadStatus status;
            status.type = loadType;
            status.PackProfileIndex = componentIndex;
            d->remoteLoadStatusList.append(status);
            taskIndex++;
        }
        componentIndex++;
    }
    d->remoteTasksInProgress = taskIndex;
    switch (result) {
        case LoadResult::LoadedLocal: {
            // Everything got loaded. Advance to dependency resolution.
            resolveDependencies(d->mode == Mode::Launch || d->netmode == Net::Mode::Offline);
            break;
        }
        case LoadResult::RequiresRemote: {
            // we wait for signals.
            break;
        }
        case LoadResult::Failed: {
            emitFailed(tr("Some component metadata load tasks failed."));
            break;
        }
    }
}

namespace {
struct RequireEx : public Meta::Require {
    size_t indexOfFirstDependee = 0;
};
struct RequireCompositionResult {
    bool ok;
    RequireEx outcome;
};
using RequireExSet = std::set<RequireEx>;
}  // namespace

static RequireCompositionResult composeRequirement(const RequireEx& a, const RequireEx& b)
{
    assert(a.uid == b.uid);
    RequireEx out;
    out.uid = a.uid;
    out.indexOfFirstDependee = std::min(a.indexOfFirstDependee, b.indexOfFirstDependee);
    if (a.equalsVersion.isEmpty()) {
        out.equalsVersion = b.equalsVersion;
    } else if (b.equalsVersion.isEmpty()) {
        out.equalsVersion = a.equalsVersion;
    } else if (a.equalsVersion == b.equalsVersion) {
        out.equalsVersion = a.equalsVersion;
    } else {
        // FIXME: mark error as explicit version conflict
        return { false, out };
    }

    if (a.suggests.isEmpty()) {
        out.suggests = b.suggests;
    } else if (b.suggests.isEmpty()) {
        out.suggests = a.suggests;
    } else {
        Version aVer(a.suggests);
        Version bVer(b.suggests);
        out.suggests = (aVer < bVer ? b.suggests : a.suggests);
    }
    return { true, out };
}

// gather the requirements from all components, finding any obvious conflicts
static bool gatherRequirementsFromComponents(const ComponentContainer& input, RequireExSet& output)
{
    bool succeeded = true;
    size_t componentNum = 0;
    for (auto component : input) {
        auto& componentRequires = component->m_cachedRequires;
        for (const auto& componentRequire : componentRequires) {
            auto found = std::find_if(output.cbegin(), output.cend(),
                                      [componentRequire](const Meta::Require& req) { return req.uid == componentRequire.uid; });

            RequireEx componenRequireEx;
            componenRequireEx.uid = componentRequire.uid;
            componenRequireEx.suggests = componentRequire.suggests;
            componenRequireEx.equalsVersion = componentRequire.equalsVersion;
            componenRequireEx.indexOfFirstDependee = componentNum;

            if (found != output.cend()) {
                // found... process it further
                auto result = composeRequirement(componenRequireEx, *found);
                if (result.ok) {
                    output.erase(componenRequireEx);
                    output.insert(result.outcome);
                } else {
                    qCritical() << "Conflicting requirements:" << componentRequire.uid << "versions:" << componentRequire.equalsVersion
                                << ";" << (*found).equalsVersion;
                }
                succeeded &= result.ok;
            } else {
                // not found, accumulate
                output.insert(componenRequireEx);
            }
        }
        componentNum++;
    }
    return succeeded;
}

/// Get list of uids that can be trivially removed because nothing is depending on them anymore (and they are installed as deps)
static void getTrivialRemovals(const ComponentContainer& components, const RequireExSet& reqs, QStringList& toRemove)
{
    for (const auto& component : components) {
        if (!component->m_dependencyOnly)
            continue;
        if (!component->m_cachedVolatile)
            continue;
        RequireEx reqNeedle;
        reqNeedle.uid = component->m_uid;
        const auto iter = reqs.find(reqNeedle);
        if (iter == reqs.cend()) {
            toRemove.append(component->m_uid);
        }
    }
}

/**
 * handles:
 * - trivial addition (there is an unmet requirement and it can be trivially met by adding something)
 * - trivial version conflict of dependencies == explicit version required and installed is different
 *
 * toAdd - set of requirements than mean adding a new component
 * toChange - set of requirements that mean changing version of an existing component
 */
static bool getTrivialComponentChanges(const ComponentIndex& index, const RequireExSet& input, RequireExSet& toAdd, RequireExSet& toChange)
{
    enum class Decision { Undetermined, Met, Missing, VersionNotSame, LockedVersionNotSame } decision = Decision::Undetermined;

    QString reqStr;
    bool succeeded = true;
    // list the composed requirements and say if they are met or unmet
    for (auto& req : input) {
        do {
            if (req.equalsVersion.isEmpty()) {
                reqStr = QString("Req: %1").arg(req.uid);
                if (index.contains(req.uid)) {
                    decision = Decision::Met;
                } else {
                    toAdd.insert(req);
                    decision = Decision::Missing;
                }
                break;
            } else {
                reqStr = QString("Req: %1 == %2").arg(req.uid, req.equalsVersion);
                const auto& compIter = index.find(req.uid);
                if (compIter == index.cend()) {
                    toAdd.insert(req);
                    decision = Decision::Missing;
                    break;
                }
                auto& comp = (*compIter);
                if (comp->getVersion() != req.equalsVersion) {
                    if (comp->isCustom()) {
                        decision = Decision::LockedVersionNotSame;
                    } else {
                        if (comp->m_dependencyOnly) {
                            decision = Decision::VersionNotSame;
                        } else {
                            decision = Decision::LockedVersionNotSame;
                        }
                    }
                    break;
                }
                decision = Decision::Met;
            }
        } while (false);
        switch (decision) {
            case Decision::Undetermined:
                qCritical() << "No decision for" << reqStr;
                succeeded = false;
                break;
            case Decision::Met:
                qDebug() << reqStr << "Is met.";
                break;
            case Decision::Missing:
                qDebug() << reqStr << "Is missing and should be added at" << req.indexOfFirstDependee;
                toAdd.insert(req);
                break;
            case Decision::VersionNotSame:
                qDebug() << reqStr << "already has different version that can be changed.";
                toChange.insert(req);
                break;
            case Decision::LockedVersionNotSame:
                qDebug() << reqStr << "already has different version that cannot be changed.";
                succeeded = false;
                break;
        }
    }
    return succeeded;
}

// FIXME, TODO: decouple dependency resolution from loading
// FIXME: This works directly with the PackProfile internals. It shouldn't! It needs richer data types than PackProfile uses.
// FIXME: throw all this away and use a graph
void ComponentUpdateTask::resolveDependencies(bool checkOnly)
{
    qDebug() << "Resolving dependencies";
    /*
     * this is a naive dependency resolving algorithm. all it does is check for following conditions and react in simple ways:
     * 1. There are conflicting dependencies on the same uid with different exact version numbers
     *    -> hard error
     * 2. A dependency has non-matching exact version number
     *    -> hard error
     * 3. A dependency is entirely missing and needs to be injected before the dependee(s)
     *    -> requirements are injected
     *
     * NOTE: this is a placeholder and should eventually be replaced with something 'serious'
     */
    auto& components = d->m_list->d->components;
    auto& componentIndex = d->m_list->d->componentIndex;

    RequireExSet allRequires;
    QStringList toRemove;
    do {
        allRequires.clear();
        toRemove.clear();
        if (!gatherRequirementsFromComponents(components, allRequires)) {
            emitFailed(tr("Conflicting requirements detected during dependency checking!"));
            return;
        }
        getTrivialRemovals(components, allRequires, toRemove);
        if (!toRemove.isEmpty()) {
            qDebug() << "Removing obsolete components...";
            for (auto& remove : toRemove) {
                qDebug() << "Removing" << remove;
                d->m_list->remove(remove);
            }
        }
    } while (!toRemove.isEmpty());
    RequireExSet toAdd;
    RequireExSet toChange;
    bool succeeded = getTrivialComponentChanges(componentIndex, allRequires, toAdd, toChange);
    if (!succeeded) {
        emitFailed(tr("Instance has conflicting dependencies."));
        return;
    }
    if (checkOnly) {
        if (toAdd.size() || toChange.size()) {
            emitFailed(tr("Instance has unresolved dependencies while loading/checking for launch."));
        } else {
            emitSucceeded();
        }
        return;
    }

    bool recursionNeeded = false;
    if (toAdd.size()) {
        // add stuff...
        for (auto& add : toAdd) {
            auto component = makeShared<Component>(d->m_list, add.uid);
            if (!add.equalsVersion.isEmpty()) {
                // exact version
                qDebug() << "Adding" << add.uid << "version" << add.equalsVersion << "at position" << add.indexOfFirstDependee;
                component->m_version = add.equalsVersion;
            } else {
                // version needs to be decided
                qDebug() << "Adding" << add.uid << "at position" << add.indexOfFirstDependee;
                // ############################################################################################################
                // HACK HACK HACK HACK FIXME: this is a placeholder for deciding what version to use. For now, it is hardcoded.
                if (!add.suggests.isEmpty()) {
                    component->m_version = add.suggests;
                } else {
                    if (add.uid == "org.lwjgl") {
                        component->m_version = "2.9.1";
                    } else if (add.uid == "org.lwjgl3") {
                        component->m_version = "3.1.2";
                    } else if (add.uid == "net.fabricmc.intermediary" || add.uid == "org.quiltmc.hashed") {
                        auto minecraft = std::find_if(components.begin(), components.end(),
                                                      [](ComponentPtr& cmp) { return cmp->getID() == "net.minecraft"; });
                        if (minecraft != components.end()) {
                            component->m_version = (*minecraft)->getVersion();
                        }
                    }
                }
                // HACK HACK HACK HACK FIXME: this is a placeholder for deciding what version to use. For now, it is hardcoded.
                // ############################################################################################################
            }
            component->m_dependencyOnly = true;
            // FIXME: this should not work directly with the component list
            d->m_list->insertComponent(add.indexOfFirstDependee, component);
            componentIndex[add.uid] = component;
        }
        recursionNeeded = true;
    }
    if (toChange.size()) {
        // change a version of something that exists
        for (auto& change : toChange) {
            // FIXME: this should not work directly with the component list
            qDebug() << "Setting version of " << change.uid << "to" << change.equalsVersion;
            auto component = componentIndex[change.uid];
            component->setVersion(change.equalsVersion);
        }
        recursionNeeded = true;
    }

    if (recursionNeeded) {
        loadComponents();
    } else {
        emitSucceeded();
    }
}

void ComponentUpdateTask::remoteLoadSucceeded(size_t taskIndex)
{
    auto& taskSlot = d->remoteLoadStatusList[taskIndex];
    if (taskSlot.finished) {
        qWarning() << "Got multiple results from remote load task" << taskIndex;
        return;
    }
    qDebug() << "Remote task" << taskIndex << "succeeded";
    taskSlot.succeeded = false;
    taskSlot.finished = true;
    d->remoteTasksInProgress--;
    // update the cached data of the component from the downloaded version file.
    if (taskSlot.type == RemoteLoadStatus::Type::Version) {
        auto component = d->m_list->getComponent(taskSlot.PackProfileIndex);
        component->m_loaded = true;
        component->updateCachedData();
    }
    checkIfAllFinished();
}

void ComponentUpdateTask::remoteLoadFailed(size_t taskIndex, const QString& msg)
{
    auto& taskSlot = d->remoteLoadStatusList[taskIndex];
    if (taskSlot.finished) {
        qWarning() << "Got multiple results from remote load task" << taskIndex;
        return;
    }
    qDebug() << "Remote task" << taskIndex << "failed: " << msg;
    d->remoteLoadSuccessful = false;
    taskSlot.succeeded = false;
    taskSlot.finished = true;
    taskSlot.error = msg;
    d->remoteTasksInProgress--;
    checkIfAllFinished();
}

void ComponentUpdateTask::checkIfAllFinished()
{
    if (d->remoteTasksInProgress) {
        // not yet...
        return;
    }
    if (d->remoteLoadSuccessful) {
        // nothing bad happened... clear the temp load status and proceed with looking at dependencies
        d->remoteLoadStatusList.clear();
        resolveDependencies(d->mode == Mode::Launch);
    } else {
        // remote load failed... report error and bail
        QStringList allErrorsList;
        for (auto& item : d->remoteLoadStatusList) {
            if (!item.succeeded) {
                allErrorsList.append(item.error);
            }
        }
        auto allErrors = allErrorsList.join("\n");
        emitFailed(tr("Component metadata update task failed while downloading from remote server:\n%1").arg(allErrors));
        d->remoteLoadStatusList.clear();
    }
}
