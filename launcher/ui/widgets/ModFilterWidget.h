#pragma once

#include <QTabWidget>
#include <QButtonGroup>

#include "Version.h"

#include "meta/Index.h"
#include "meta/VersionList.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

class MinecraftInstance;

namespace Ui {
class ModFilterWidget;
}

class ModFilterWidget : public QTabWidget
{
    Q_OBJECT
public:
    enum VersionButtonID {
        Strict = 0,
        Major = 1,
        All = 2,
        Between = 3
    };

    struct Filter {
        std::list<Version> versions;

        bool operator==(const Filter& other) const { return versions == other.versions; }
        bool operator!=(const Filter& other) const { return !(*this == other); }
    };

    std::shared_ptr<Filter> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filter;

public:
    static unique_qobject_ptr<ModFilterWidget> create(Version default_version, QWidget* parent = nullptr);
    ~ModFilterWidget();

    void setInstance(MinecraftInstance* instance);

    /// By default all buttons are enabled
    void disableVersionButton(VersionButtonID, QString reason = {});

    auto getFilter() -> std::shared_ptr<Filter>;
    auto changed() const -> bool { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_last_version_id != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_id; }

    Meta::VersionList::Ptr versionList() { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_list; }

private:
    ModFilterWidget(Version def, QWidget* parent = nullptr);

    inline auto mcVersionStr() const -> QString { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance ? hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->getPackProfile()->getComponentVersion("net.minecraft") : ""; }
    inline auto mcVersion() const -> Version { return { mcVersionStr() }; }

private slots:
    void onVersionFilterChanged(int id);

public: signals:
    void filterChanged();
    void filterUnchanged();

private:
    Ui::ModFilterWidget* ui;

    MinecraftInstance* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance = nullptr;


/* Version stuff */
    QButtonGroup hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mcVersion_buttons;

    Meta::VersionList::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_list;

    /* Used to tell if the filter was changed since the last getFilter() call */
    VersionButtonID hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_last_version_id = VersionButtonID::Strict;
    VersionButtonID hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_id = VersionButtonID::Strict;
};
