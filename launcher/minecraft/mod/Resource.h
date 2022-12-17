#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QPointer>

#include "QObjectPtr.h"

enum class ResourceType {
    UNKNOWN,     //!< Indicates an unspecified resource type.
    ZIPFILE,     //!< The resource is a zip file containing the resource's class files.
    SINGLEFILE,  //!< The resource is a single file (not a zip file).
    FOLDER,      //!< The resource is in a folder on the filesystem.
    LITEMOD,     //!< The resource is a litemod
};

enum class SortType {
    NAME,
    DATE,
    VERSION,
    ENABLED,
    PACK_FORMAT
};

enum class EnableAction {
    ENABLE,
    DISABLE,
    TOGGLE
};

/** General class for managed resources. It mirrors a file in disk, with some more info
 *  for display and house-keeping purposes.
 *
 *  Subclass it to add additional data / behavior, such as Mods or Resource packs.
 */
class Resource : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(Resource)
   public:
    using Ptr = shared_qobject_ptr<Resource>;
    using WeakPtr = QPointer<Resource>;

    Resource(QObject* parent = nullptr);
    Resource(QFileInfo file_info);
    Resource(QString file_path) : Resource(QFileInfo(file_path)) {}

    ~Resource() override = default;

    void setFile(QFileInfo file_info);
    void parseFile();

    [[nodiscard]] auto fileinfo() const -> QFileInfo { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info; }
    [[nodiscard]] auto dateTimeChanged() const -> QDateTime { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_changed_date_time; }
    [[nodiscard]] auto internal_id() const -> QString { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_internal_id; }
    [[nodiscard]] auto type() const -> ResourceType { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type; }
    [[nodiscard]] bool enabled() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled; }

    [[nodiscard]] virtual auto name() const -> QString { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name; }
    [[nodiscard]] virtual bool valid() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type != ResourceType::UNKNOWN; }

    /** Compares two Resources, for sorting purposes, considering a ascending order, returning:
     *  > 0: 'this' comes after 'other'
     *  = 0: 'this' is equal to 'other'
     *  < 0: 'this' comes before 'other'
     *
     *  The second argument in the pair is true if the sorting type that decided which one is greater was 'type'.
     */
    [[nodiscard]] virtual auto compare(Resource const& other, SortType type = SortType::NAME) const -> std::pair<int, bool>;

    /** Returns whether the given filter should filter out 'this' (false),
     *  or if such filter includes the Resource (true).
     */
    [[nodiscard]] virtual bool applyFilter(QRegularExpression filter) const;

    /** Changes the enabled property, according to 'action'.
     *
     *  Returns whether a change was applied to the Resource's properties.
     */
    bool enable(EnableAction action);

    [[nodiscard]] auto shouldResolve() const -> bool { return !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolving && !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolved; }
    [[nodiscard]] auto isResolving() const -> bool { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolving; }
    [[nodiscard]] auto isResolved() const -> bool { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolved; }
    [[nodiscard]] auto resolutionTicket() const -> int { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resolution_ticket; }

    void setResolving(bool resolving, int resolutionTicket)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolving = resolving;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resolution_ticket = resolutionTicket;
    }

    // Delete all files of this resource.
    bool destroy();

   protected:
    /* The file corresponding to this resource. */
    QFileInfo hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info;
    /* The cached date when this file was last changed. */
    QDateTime hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_changed_date_time;

    /* Internal ID for internal purposes. Properties such as human-readability should not be assumed. */
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_internal_id;
    /* Name as reported via the file name. In the absence of a better name, this is shown to the user. */
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;

    /* The type of file we're dealing with. */
    ResourceType hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::UNKNOWN;

    /* Whether the resource is enabled (e.g. shows up in the game) or not. */
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled = true;

    /* Used to keep trach of pending / concluded actions on the resource. */
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolving = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_resolved = false;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resolution_ticket = 0;
};
