#pragma once

#include <QDialog>
#include <QVBoxLayout>

#include "BaseVersion.h"
#include "ui/pages/BasePageProvider.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ModDownloadTask.h"
#include "ui/pages/modplatform/flame/FlameModPage.h"

namespace Ui
{
class ModDownloadDialog;
}

class PageContainer;
class QDialogButtonBox;
class ModrinthPage;

class ModDownloadDialog : public QDialog, public BasePageProvider
{
    Q_OBJECT

public:
    explicit ModDownloadDialog(const std::shared_ptr<ModFolderModel> &mods, QWidget *parent, BaseInstance *instance);
    ~ModDownloadDialog();

    QString dialogTitle() override;
    QList<BasePage *> getPages() override;

    void setSuggestedMod(const QString & name = QString(), ModDownloadTask * task = nullptr);

    ModDownloadTask * getTask();
    const std::shared_ptr<ModFolderModel> &mods;

public slots:
    void accept() override;
    void reject() override;

//private slots:

private:
    Ui::ModDownloadDialog *ui = nullptr;
    PageContainer * m_container = nullptr;
    QDialogButtonBox * m_buttons = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;


    ModrinthPage *modrinthPage = nullptr;
    FlameModPage *flameModPage = nullptr;
    std::unique_ptr<ModDownloadTask> modTask;
    BaseInstance *m_instance;
};
