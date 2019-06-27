#include "TwitchPage.h"
#include "ui_TwitchPage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"

TwitchPage::TwitchPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::TwitchPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->checkButton, &QPushButton::clicked, this, &TwitchPage::triggerCheck);
}

TwitchPage::~TwitchPage()
{
    delete ui;
}

bool TwitchPage::shouldDisplay() const
{
    return true;
}

void TwitchPage::openedImpl()
{
    dialog->setSuggestedPack();
}

void TwitchPage::triggerCheck(bool)
{
    if(m_modIdResolver) {
        qDebug() << "Click!";
        return;
    }
    auto task = new Flame::UrlResolvingTask(ui->lineEdit->text());
    connect(task, &Task::finished, this, &TwitchPage::checkDone);
    m_modIdResolver.reset(task);
    task->start();
}

void TwitchPage::checkDone()
{
    auto result = m_modIdResolver->getResults();
    auto formatted = QString("Project %1, File %2").arg(result.projectId).arg(result.fileId);
    ui->twitchLabel->setText(formatted);
    m_modIdResolver.reset();
}
