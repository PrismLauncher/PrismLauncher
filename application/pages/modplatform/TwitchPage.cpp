#include "TwitchPage.h"
#include "ui_TwitchPage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"
#include <InstanceImportTask.h>

TwitchPage::TwitchPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::TwitchPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->checkButton, &QPushButton::clicked, this, &TwitchPage::triggerCheck);
    connect(ui->twitchLabel, &DropLabel::droppedURLs, [this](QList<QUrl> urls){
        if(urls.size()) {
            setUrl(urls[0].toString());
        }
    });
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
        return;
    }
    auto task = new Flame::UrlResolvingTask(ui->lineEdit->text());
    connect(task, &Task::finished, this, &TwitchPage::checkDone);
    m_modIdResolver.reset(task);
    task->start();
}

void TwitchPage::setUrl(const QString& url)
{
    ui->lineEdit->setText(url);
    triggerCheck(true);
}

void TwitchPage::checkDone()
{
    auto result = m_modIdResolver->getResults();
    auto formatted = QString("Project %1, File %2").arg(result.projectId).arg(result.fileId);
    if(result.resolved && result.type == Flame::File::Type::Modpack) {
        ui->twitchLabel->setText(formatted);
        QFileInfo fi(result.fileName);
        dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(result.url));
    } else {
        ui->twitchLabel->setPixmap(QPixmap(QString::fromUtf8(":/assets/deadglitch")));
        dialog->setSuggestedPack();
    }
    m_modIdResolver.reset();
}
