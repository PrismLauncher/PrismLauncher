#include "GameOptionsPage.h"
#include "ui_GameOptionsPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/gameoptions/GameOptions.h"

GameOptionsPage::GameOptionsPage(MinecraftInstance * inst, QWidget* parent)
    : QWidget(parent), ui(new Ui::GameOptionsPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    m_model = inst->gameOptionsModel();
    ui->optionsView->setModel(m_model.get());
    auto head = ui->optionsView->header();
    if(head->count())
    {
        head->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        for(int i = 1; i < head->count(); i++)
        {
            head->setSectionResizeMode(i, QHeaderView::Stretch);
        }
    }
}

GameOptionsPage::~GameOptionsPage()
{
    // m_model->save();
}

void GameOptionsPage::openedImpl()
{
    // m_model->observe();
}

void GameOptionsPage::closedImpl()
{
    // m_model->unobserve();
}

void GameOptionsPage::retranslate()
{
    ui->retranslateUi(this);
}
