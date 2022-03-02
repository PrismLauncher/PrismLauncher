#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>
#include "tasks/Task.h"
#include "modplatform/flame/FlameModIndex.h"

namespace Ui
{
class FlameModPage;
}

class ModDownloadDialog;

namespace FlameMod {
    class ListModel;
}

class FlameModPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit FlameModPage(ModDownloadDialog *dialog, BaseInstance *instance);
    virtual ~FlameModPage();
    virtual QString displayName() const override
    {
        return tr("CurseForge");
    }
    virtual QIcon icon() const override
    {
        return APPLICATION->getThemedIcon("flame");
    }
    virtual QString id() const override
    {
        return "curseforge";
    }
    virtual QString helpPage() const override
    {
        return "Flame-platform";
    }
    virtual bool shouldDisplay() const override;

    void openedImpl() override;

    bool eventFilter(QObject * watched, QEvent * event) override;

    BaseInstance *m_instance;

private:
    void updateSelectionButton();

private slots:
    void triggerSearch();
    void onSelectionChanged(QModelIndex first, QModelIndex second);
    void onVersionSelectionChanged(QString data);
    void onModSelected();

private:
    Ui::FlameModPage *ui = nullptr;
    ModDownloadDialog* dialog = nullptr;
    FlameMod::ListModel* listModel = nullptr;
    ModPlatform::IndexedPack current;

    int selectedVersion = -1;
};
