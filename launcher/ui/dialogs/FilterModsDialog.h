#pragma once

#include <QDialog>
#include <QButtonGroup>

#include "Version.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

class MinecraftInstance;

namespace Ui {
class FilterModsDialog;
}

class FilterModsDialog : public QDialog
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
    };

    std::shared_ptr<Filter> m_filter;

public:
    explicit FilterModsDialog(Version def, QWidget* parent = nullptr);
    ~FilterModsDialog();

    int execWithInstance(MinecraftInstance* instance);

    /// By default all buttons are enabled
    void disableVersionButton(VersionButtonID);

    auto getFilter() -> std::shared_ptr<Filter> { return m_filter; }

private:
    inline auto mcVersionStr() const -> QString { return m_instance ? m_instance->getPackProfile()->getComponentVersion("net.minecraft") : ""; }
    inline auto mcVersion() const -> Version { return { mcVersionStr() }; }

private slots:
    void onVersionFilterChanged(int id);

private:
    Ui::FilterModsDialog* ui;

    MinecraftInstance* m_instance = nullptr;

    QButtonGroup m_mcVersion_buttons;
};
