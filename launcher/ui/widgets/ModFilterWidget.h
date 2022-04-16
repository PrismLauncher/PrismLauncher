#pragma once

#include <QTabWidget>
#include <QButtonGroup>

#include "Version.h"
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

    std::shared_ptr<Filter> m_filter;

public:
    explicit ModFilterWidget(Version def, QWidget* parent = nullptr);
    ~ModFilterWidget();

    void setInstance(MinecraftInstance* instance);

    /// By default all buttons are enabled
    void disableVersionButton(VersionButtonID);

    auto getFilter() -> std::shared_ptr<Filter>;
    auto changed() const -> bool { return m_last_version_id != m_version_id; }

private:
    inline auto mcVersionStr() const -> QString { return m_instance ? m_instance->getPackProfile()->getComponentVersion("net.minecraft") : ""; }
    inline auto mcVersion() const -> Version { return { mcVersionStr() }; }

private slots:
    void onVersionFilterChanged(int id);

public: signals:
    void filterChanged();
    void filterUnchanged();

private:
    Ui::ModFilterWidget* ui;

    MinecraftInstance* m_instance = nullptr;

    QButtonGroup m_mcVersion_buttons;

    /* Used to tell if the filter was changed since the last getFilter() call */
    VersionButtonID m_last_version_id = VersionButtonID::Strict;
    VersionButtonID m_version_id = VersionButtonID::Strict;
};
