#pragma once

#include <memory>
#include <QList>
#include <QJsonDocument>
#include <QDateTime>
#include "meta/JsonFormat.h"
#include "ProblemProvider.h"
#include "QObjectPtr.h"

class PackProfile;
class LaunchProfile;
namespace Meta
{
    class Version;
    class VersionList;
}
class VersionFile;

class Component : public QObject, public ProblemProvider
{
Q_OBJECT
public:
    Component(PackProfile * parent, const QString &uid);

    // DEPRECATED: remove these constructors?
    Component(PackProfile * parent, std::shared_ptr<Meta::Version> version);
    Component(PackProfile * parent, const QString & uid, std::shared_ptr<VersionFile> file);

    virtual ~Component(){};
    void applyTo(LaunchProfile *profile);

    bool isEnabled();
    bool setEnabled (bool state);
    bool canBeDisabled();

    bool isMoveable();
    bool isCustomizable();
    bool isRevertible();
    bool isRemovable();
    bool isCustom();
    bool isVersionChangeable();

    // DEPRECATED: explicit numeric order values, used for loading old non-component config. TODO: refactor and move to migration code
    void setOrder(int order);
    int getOrder();

    QString getID();
    QString getName();
    QString getVersion();
    std::shared_ptr<Meta::Version> getMeta();
    QDateTime getReleaseDateTime();

    QString getFilename();

    std::shared_ptr<class VersionFile> getVersionFile() const;
    std::shared_ptr<class Meta::VersionList> getVersionList() const;

    void setImportant (bool state);


    const QList<PatchProblem> getProblems() const override;
    ProblemSeverity getProblemSeverity() const override;

    void setVersion(const QString & version);
    bool customize();
    bool revert();

    void updateCachedData();

signals:
    void dataChanged();

public: /* data */
    PackProfile * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent;

    // BEGIN: persistent component list properties
    /// ID of the component
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
    /// version of the component - when there's a custom json override, this is also the version the component reverts to
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
    /// if true, this has been added automatically to satisfy dependencies and may be automatically removed
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dependencyOnly = false;
    /// if true, the component is either the main component of the instance, or otherwise important and cannot be removed.
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_important = false;
    /// if true, the component is disabled
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_disabled = false;

    /// cached name for display purposes, taken from the version file (meta or local override)
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName;
    /// cached version for display AND other purposes, taken from the version file (meta or local override)
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion;
    /// cached set of requirements, taken from the version file (meta or local override)
    Meta::RequireSet hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedRequires;
    Meta::RequireSet hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedConflicts;
    /// if true, the component is volatile and may be automatically removed when no longer needed
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVolatile = false;
    // END: persistent component list properties

    // DEPRECATED: explicit numeric order values, used for loading old non-component config. TODO: refactor and move to migration code
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orderOverride = false;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_order = 0;

    // load state
    std::shared_ptr<Meta::Version> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion;
    std::shared_ptr<VersionFile> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded = false;
};

typedef shared_qobject_ptr<Component> ComponentPtr;
