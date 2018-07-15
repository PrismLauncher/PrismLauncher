#include "TwitchPage.h"
#include "ui_TwitchPage.h"

#include "MultiMC.h"
#include "FolderInstanceProvider.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ProgressDialog.h"
#include "dialogs/NewInstanceDialog.h"

TwitchPage::TwitchPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::TwitchPage), dialog(dialog)
{
    ui->setupUi(this);
}

TwitchPage::~TwitchPage()
{
    delete ui;
}

bool TwitchPage::shouldDisplay() const
{
    return false;
}

void TwitchPage::openedImpl()
{
    dialog->setSuggestedPack();
}
