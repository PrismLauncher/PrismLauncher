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

/* This page handles most logic related to browsing and selecting mods to download.
 * By default, the methods provided work with net requests, to fetch data from remote APIs. */
class ModPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ModPage(ModDownloadDialog* dialog, BaseInstance* instance, ModAPI* api);
    virtual ~ModPage();

    /* The name visible to the user */
    virtual QString displayName() const override = 0;
    virtual QIcon icon() const override = 0;
    virtual QString id() const override = 0;
    virtual QString helpPage() const override = 0;

    virtual QString metaEntryBase() const = 0;
    /* This only appears in the debug console */
    virtual QString debugName() const = 0;

    virtual bool shouldDisplay() const override = 0;
    const ModAPI* apiProvider() const { return api.get(); };

    virtual void onGetVersionsSucceeded(ModPage*, QByteArray*, QString) = 0;

    void openedImpl() override;
    bool eventFilter(QObject* watched, QEvent* event) override;

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
