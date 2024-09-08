// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QTimer>
#include <QWidget>

#include "ResourceDownloadTask.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include "ui/pages/BasePage.h"
#include "ui/pages/modplatform/ResourceModel.h"
#include "ui/widgets/ProgressWidget.h"

namespace Ui {
class ResourcePage;
}

class BaseInstance;

namespace ResourceDownload {

class ResourceDownloadDialog;
class ResourceModel;

class ResourcePage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    using DownloadTaskPtr = shared_qobject_ptr<ResourceDownloadTask>;
    ~ResourcePage() override;

    /* Affects what the user sees */
    [[nodiscard]] auto displayName() const -> QString override = 0;
    [[nodiscard]] auto icon() const -> QIcon override = 0;
    [[nodiscard]] auto id() const -> QString override = 0;
    [[nodiscard]] auto helpPage() const -> QString override = 0;
    [[nodiscard]] bool shouldDisplay() const override = 0;

    /* Used internally */
    [[nodiscard]] virtual auto metaEntryBase() const -> QString = 0;
    [[nodiscard]] virtual auto debugName() const -> QString = 0;

    //: The plural version of 'resource'
    [[nodiscard]] virtual inline QString resourcesString() const { return tr("resources"); }
    //: The singular version of 'resources'
    [[nodiscard]] virtual inline QString resourceString() const { return tr("resource"); }

    /* Features this resource's page supports */
    [[nodiscard]] virtual bool supportsFiltering() const = 0;

    void retranslate() override;
    void openedImpl() override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

    /** Get the current term in the search bar. */
    [[nodiscard]] auto getSearchTerm() const -> QString;
    /** Programatically set the term in the search bar. */
    void setSearchTerm(QString);

    [[nodiscard]] bool setCurrentPack(ModPlatform::IndexedPack::Ptr);
    [[nodiscard]] auto getCurrentPack() const -> ModPlatform::IndexedPack::Ptr;
    [[nodiscard]] auto getDialog() const -> const ResourceDownloadDialog* { return m_parent_dialog; }
    [[nodiscard]] auto getModel() const -> ResourceModel* { return m_model; }

   protected:
    ResourcePage(ResourceDownloadDialog* parent, BaseInstance&);

    void addSortings();

   public slots:
    virtual void updateUi();
    virtual void updateSelectionButton();
    virtual void updateVersionList();

    void addResourceToDialog(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&);
    void removeResourceFromDialog(const QString& pack_name);
    virtual void removeResourceFromPage(const QString& name);
    virtual void addResourceToPage(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&, std::shared_ptr<ResourceFolderModel>);

    QList<DownloadTaskPtr> selectedPacks() { return m_model->selectedPacks(); }
    bool hasSelectedPacks() { return !(m_model->selectedPacks().isEmpty()); }

    virtual void openProject(QVariant projectID);

   protected slots:
    virtual void triggerSearch() = 0;

    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(int index);
    void onResourceSelected();

    // NOTE: Can't use [[nodiscard]] here because of https://bugreports.qt.io/browse/QTBUG-58628 on Qt 5.12

    /** Associates regex expressions to pages in the order they're given in the map. */
    virtual QMap<QString, QString> urlHandlers() const = 0;
    virtual void openUrl(const QUrl&);

   public:
    BaseInstance& m_base_instance;

   protected:
    Ui::ResourcePage* m_ui;

    ResourceDownloadDialog* m_parent_dialog = nullptr;
    ResourceModel* m_model = nullptr;

    int m_selected_version_index = -1;

    ProgressWidget m_fetch_progress;

    // Used to do instant searching with a delay to cache quick changes
    QTimer m_search_timer;

    bool m_do_not_jump_to_mod = false;
};

}  // namespace ResourceDownload
