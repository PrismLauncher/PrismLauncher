// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QTimer>
#include <QWidget>

#include "ResourceDownloadTask.h"
#include "modplatform/ModIndex.h"

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

struct ResourceProviderData {
    QString displayName;
    QIcon icon;
    QString id;
    QString metaEntryBase;
    QString debugName;
};

struct ResourceDescriptor {
    QString helpPage;
    QString resourceString = QObject::tr("resource");
    QString resourcesString = QObject::tr("resources");

    bool supportsFiltering = false;
    bool isIndexed = true;
    QMap<QString, QString> urlHandlers;
};

class ResourcePage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    using DownloadTaskPtr = shared_qobject_ptr<ResourceDownloadTask>;
    ~ResourcePage() override;

    /* Affects what the user sees */
    auto displayName() const -> QString override { return m_provider.displayName; };
    auto icon() const -> QIcon override { return m_provider.icon; };
    auto id() const -> QString override { return m_provider.id; };
    auto helpPage() const -> QString override { return m_desc.helpPage; };
    bool shouldDisplay() const override { return true; };

    /* Used internally */
    virtual auto metaEntryBase() const -> QString { return m_provider.metaEntryBase; };
    virtual auto debugName() const -> QString { return m_provider.debugName; };

    //: The plural version of 'resource'
    virtual QString resourcesString() const { return m_desc.resourcesString; }
    //: The singular version of 'resources'
    virtual QString resourceString() const { return m_desc.resourceString; }

    /* Features this resource's page supports */
    virtual bool supportsFiltering() const { return m_desc.supportsFiltering; };

    void retranslate() override;
    void openedImpl() override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

    /** Get the current term in the search bar. */
    auto getSearchTerm() const -> QString;
    /** Programatically set the term in the search bar. */
    void setSearchTerm(const QString&);

    bool setCurrentPack(ModPlatform::IndexedPack::Ptr);
    auto getCurrentPack() const -> ModPlatform::IndexedPack::Ptr;
    auto getDialog() const -> const ResourceDownloadDialog* { return m_parentDialog; }
    auto getModel() const -> ResourceModel* { return m_model; }

   protected:
    ResourcePage(ResourceDownloadDialog* parent,
                 BaseInstance& baseInstance,
                 ResourceDescriptor desc = {},
                 ResourceProviderData provider = {});

    void addSortings();

   public slots:
    virtual void updateUi(const QModelIndex& index);
    virtual void updateSelectionButton();
    virtual void versionListUpdated(const QModelIndex& index);

    void addResourceToDialog(ModPlatform::IndexedPack::Ptr, ModPlatform::IndexedVersion&);
    void removeResourceFromDialog(const QString& packName);
    virtual void removeResourceFromPage(const QString& name);
    virtual void addResourceToPage(ModPlatform::IndexedPack::Ptr,
                                   ModPlatform::IndexedVersion&,
                                   ResourceFolderModel*,
                                   QString downloadReason = "standalone",
                                   QString dependentOn = "");

    virtual void modelReset();

    QList<DownloadTaskPtr> selectedPacks() { return m_model->selectedPacks(); }
    bool hasSelectedPacks() { return !(m_model->selectedPacks().isEmpty()); }

    virtual void openProject(const QVariant& projectID);

    void setSuppressInitialSearch(bool suppress);

   protected slots:
    virtual void triggerSearch() = 0;

    void onSelectionChanged(QModelIndex curr, QModelIndex prev);
    void onVersionSelectionChanged(int index);
    void onResourceSelected();
    void onResourceToggle(const QModelIndex& index);

    /** Associates regex expressions to pages in the order they're given in the map. */
    virtual QMap<QString, QString> urlHandlers() const { return m_desc.urlHandlers; };
    virtual void openUrl(const QUrl&);

   public:
    BaseInstance& m_baseInstance;

   protected:
    Ui::ResourcePage* m_ui;

    ResourceDownloadDialog* m_parentDialog = nullptr;
    ResourceModel* m_model = nullptr;

    int m_selectedVersionIndex = -1;

    ProgressWidget m_fetchProgress;

    // Used to do instant searching with a delay to cache quick changes
    QTimer m_searchTimer;

    bool m_doNotJumpToMod = false;

    QSet<int> m_enableQueue;

    ResourceDescriptor m_desc;
    ResourceProviderData m_provider;

   private:
    bool m_suppressInitialSearch = false;
};

}  // namespace ResourceDownload
