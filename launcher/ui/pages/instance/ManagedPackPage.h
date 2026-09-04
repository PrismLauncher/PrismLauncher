// SPDX-FileCopyrightText: 2022 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "minecraft/MinecraftInstance.h"

#include "modplatform/ModIndex.h"

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
    static ManagedPackPage* createPage(MinecraftInstance* inst, QWidget* parent = nullptr)
    {
        return ManagedPackPage::createPage(inst, inst->getManagedPackType(), parent);
    }

    static ManagedPackPage* createPage(MinecraftInstance* inst, const QString& type, QWidget* parent = nullptr);
    ~ManagedPackPage() override;

    QString displayName() const override;
    QIcon icon() const override;
    QString helpPage() const override;
    QString id() const override { return "managed_pack"; }
    bool shouldDisplay() const override;

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
    ManagedPackPage(MinecraftInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);

    /** Run the InstanceTask, with a progress dialog and all.
     *  Similar to MainWindow::instanceFromInstanceTask
     *
     *  Returns whether the task was successful.
     */
    bool runUpdateTask(InstanceTask*);

    void updatePack(const QUrl& url, bool trusted, const QString& versionID = {}, const QString& versionName = {});

    void onUpdateTaskCompleted(bool didSucceed) const;

   protected:
    InstanceWindow* m_instanceWindow = nullptr;

    Ui::ManagedPackPage* m_ui;
    MinecraftInstance* m_inst;

    bool m_loaded = false;
};

/** Simple page for when we aren't a managed pack. */
class GenericManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    GenericManagedPackPage(MinecraftInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr)
        : ManagedPackPage(inst, instanceWindow, parent)
    {}
    ~GenericManagedPackPage() override = default;

    // TODO: We may want to show this page with some useful info at some point.
    bool shouldDisplay() const override { return false; };
};

class ModrinthManagedPackPage final : public ManagedPackPage {
    Q_OBJECT

   public:
    ModrinthManagedPackPage(MinecraftInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);
    ~ModrinthManagedPackPage() override = default;

    void parseManagedPack() override;
    QString url() const override;
    QString helpPage() const override { return "modrinth-managed-pack"; }

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
    FlameManagedPackPage(MinecraftInstance* inst, InstanceWindow* instanceWindow, QWidget* parent = nullptr);
    ~FlameManagedPackPage() override = default;

    void parseManagedPack() override;
    QString url() const override;
    QString helpPage() const override { return "curseforge-managed-pack"; }

   public slots:
    void suggestVersion() override;

    void update() override;
    void updateFromFile() override;

   private:
    Task::Ptr m_fetchJob = nullptr;

    ModPlatform::IndexedPack m_pack;
};
