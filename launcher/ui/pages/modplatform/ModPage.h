#pragma once

#include <QWidget>

#include "Application.h"
#include "modplatform/ModAPI.h"
#include "modplatform/ModIndex.h"
#include "ui/pages/BasePage.h"
#include "ui/pages/modplatform/ModModel.h"

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

    auto shouldDisplay() const -> bool override = 0;
    virtual auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, QString loaderVer = "") const -> bool = 0;

    auto apiProvider() const -> const ModAPI* { return api.get(); };

    auto getCurrent() -> ModPlatform::IndexedPack& { return current; }
    void updateModVersions();

    void openedImpl() override;
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;

    BaseInstance* m_instance;

   protected:
    void updateSelectionButton();

   protected slots:
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);
    void onModSelected();

   protected:
    Ui::ModPage* ui = nullptr;
    ModDownloadDialog* dialog = nullptr;
    ModPlatform::ListModel* listModel = nullptr;
    ModPlatform::IndexedPack current;

    std::unique_ptr<ModAPI> api;

    int selectedVersion = -1;
};
