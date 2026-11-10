// SPDX-FileCopyrightText: 2022 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "BaseInstance.h"

#include "modplatform/ModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"

#include "modplatform/flame/FlameAPI.h"

#include "ui/pages/BasePage.h"

#include <QWidget>

namespace Ui {
class ManagedPackPage;
}

class InstanceTask;
class InstanceWindow;

class ManagedPackPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    static PageInfo page(BaseInstance* inst) { return ManagedPackPage::page(inst, inst->getManagedPackType()); }

    static PageInfo page(BaseInstance* inst, const QString& type);

    ~ManagedPackPage() override;

    void openedImpl() override;

    bool apply() override { return true; }
    void retranslate() override;

    /** Gets the necessary information about the managed pack, such as
     *  available versions*/
    virtual void parseManagedPack() {};

    /** URL of the managed pack.
     *  Not the version-specific one.
     */
    virtual QString url() const { return {}; };

    void setInstanceWindow(InstanceWindow* window) { m_instanceWindow = window; }

   public slots:
    /** Gets the current version selection and update the UI, including the update button and the changelog.
     */
    virtual void suggestVersion();

    virtual void update() {};
    virtual void updateFromFile() {};
    void updateFromUrl();

   protected slots:
    /** Does the necessary UI changes for when something failed.
     *
     *  This includes:
     *  - Setting an appropriate text on the version selector to indicate a fail;
     *  - Setting an appropriate text on the changelog text browser to indicate a fail;
     *  - Disable the update button.
     */
    void setFailState();

   protected:
    ManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);

    /** Run the InstanceTask, with a progress dialog and all.
     *  Similar to MainWindow::instanceFromInstanceTask
     *
     *  Returns whether the task was successful.
     */
    bool runUpdateTask(InstanceTask*);

    void updatePack(const QUrl& url, bool trusted, const QString& versionID = {}, const QString& versionName = {});

    void onUpdateTaskCompleted(bool didSucceed) const;

    virtual QString displayName() const = 0;

   protected:
    InstanceWindow* m_instanceWindow = nullptr;

    Ui::ManagedPackPage* ui;
    BaseInstance* m_inst;

    bool m_loaded = false;
};

class ModrinthManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    static PageInfo page(BaseInstance* inst, InstanceWindow* instanceWindow)
    {
        return {
            .id = "managed_pack",
            .displayName = "Modrinth",
            .icon = QIcon::fromTheme("modrinth"),
            .helpPage = "modrinth-managed-pack",
            .constructor = [inst, instanceWindow](QWidget* parent) { return new ModrinthManagedPackPage(inst, instanceWindow, parent); },
            .shouldDisplay = [inst] { return inst->isManagedPack(); },
        };
    }

    ModrinthManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);
    ~ModrinthManagedPackPage() override = default;

    void parseManagedPack() override;
    QString url() const override;

   protected:
    QString displayName() const override { return "Modrinth"; }

   public slots:
    void suggestVersion() override;

    void update() override;
    void updateFromFile() override;

   private:
    Task::Ptr m_fetchJob = nullptr;

    ModPlatform::IndexedPack m_pack;
};

class FlameManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    FlameManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);
    ~FlameManagedPackPage() override = default;

    static PageInfo page(BaseInstance* inst, InstanceWindow* instanceWindow)
    {
        return {
            .id = "managed_pack",
            .displayName = "CurseForge",
            .icon = QIcon::fromTheme("flame"),
            .helpPage = "curseforge-managed-pack",
            .constructor = [inst, instanceWindow](QWidget* parent) { return new FlameManagedPackPage(inst, instanceWindow, parent); },
            .shouldDisplay = [inst] { return inst->isManagedPack(); },
        };
    }

    void parseManagedPack() override;
    QString url() const override;

   protected:
    QString displayName() const override { return "Modrinth"; }

   public slots:
    void suggestVersion() override;

    void update() override;
    void updateFromFile() override;

   private:
    Task::Ptr m_fetchJob = nullptr;

    ModPlatform::IndexedPack m_pack;
};
