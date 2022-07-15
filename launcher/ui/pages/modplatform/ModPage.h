#pragma once

#include <QWidget>

#include "Application.h"
#include "modplatform/ModAPI.h"
#include "modplatform/ModIndex.h"
#include "ui/pages/BasePage.h"
#include "ui/pages/modplatform/ModModel.h"
#include "ui/widgets/ModFilterWidget.h"
#include "ui/widgets/ProgressWidget.h"

class ModDownloadDialog;

namespace Ui {
class ModPage;
}

/* This page handles most logic related to browsing and selecting mods to download. */
class ModPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ModPage(ModDownloadDialog* dialog, BaseInstance* instance, ModAPI* api);
    ~ModPage() override;

    /* Affects what the user sees */
    auto displayName() const -> QString override = 0;
    auto icon() const -> QIcon override = 0;
    auto id() const -> QString override = 0;
    auto helpPage() const -> QString override = 0;

    /* Used internally */
    virtual auto metaEntryBase() const -> QString = 0;
    virtual auto debugName() const -> QString = 0;


    void retranslate() override;

    void updateUi();

    auto shouldDisplay() const -> bool override = 0;
    virtual auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, ModAPI::ModLoaderTypes loaders = ModAPI::Unspecified) const -> bool = 0;

    auto apiProvider() -> ModAPI* { return api.get(); };
    auto getFilter() const -> const std::shared_ptr<ModFilterWidget::Filter> { return m_filter; }
    auto getDialog() const -> const ModDownloadDialog* { return dialog; }

    /** Get the current term in the search bar. */
    auto getSearchTerm() const -> QString;
    /** Programatically set the term in the search bar. */
    void setSearchTerm(QString);

    auto getCurrent() -> ModPlatform::IndexedPack& { return current; }
    void updateModVersions(int prev_count = -1);

    void openedImpl() override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

    BaseInstance* m_instance;

   protected:
    void updateSelectionButton();

   protected slots:
    virtual void filterMods();
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);
    void onModSelected();

   protected:
    Ui::ModPage* ui = nullptr;
    ModDownloadDialog* dialog = nullptr;

    ModFilterWidget filter_widget;
    std::shared_ptr<ModFilterWidget::Filter> m_filter;

    ProgressWidget m_fetch_progress;

    ModPlatform::ListModel* listModel = nullptr;
    ModPlatform::IndexedPack current;

    std::unique_ptr<ModAPI> api;

    int selectedVersion = -1;

    // Used to do instant searching with a delay to cache quick changes
    QTimer m_search_timer;
};
