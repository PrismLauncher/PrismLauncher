#pragma once

#include <Application.h>
#include <QWidget>

#include "modplatform/ModAPI.h"
#include "modplatform/ModIndex.h"
#include "tasks/Task.h"
#include "ui/pages/BasePage.h"
#include "ui/pages/modplatform/ModModel.h"

class ModDownloadDialog;

namespace Ui {
class ModPage;
}

class ModPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ModPage(ModDownloadDialog* dialog, BaseInstance* instance);
    virtual ~ModPage();

    virtual QString displayName() const override = 0;
    virtual QIcon icon() const override = 0;
    virtual QString id() const override = 0;
    virtual QString helpPage() const override = 0;

    virtual QString debugName() const = 0;
    virtual QString metaEntryBase() const = 0;

    virtual bool shouldDisplay() const override = 0;
    virtual const ModAPI* apiProvider() const = 0;

    void openedImpl() override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    BaseInstance* m_instance;

   protected:
    virtual void onModVersionSucceed(ModPage*, QByteArray*, QString) = 0;

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

    int selectedVersion = -1;
};
