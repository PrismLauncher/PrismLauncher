#include "TwitchPage.h"
#include "ui_TwitchPage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"
#include <InstanceImportTask.h>
#include "TwitchModel.h"
#include <QKeyEvent>

TwitchPage::TwitchPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::TwitchPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &TwitchPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    model = new Twitch::ListModel(this);
    ui->packView->setModel(model);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TwitchPage::onSelectionChanged);
}

TwitchPage::~TwitchPage()
{
    delete ui;
}

bool TwitchPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool TwitchPage::shouldDisplay() const
{
    return true;
}

void TwitchPage::openedImpl()
{
    suggestCurrent();
}

void TwitchPage::triggerSearch()
{
    model->searchWithTerm(ui->searchEdit->text());
}

void TwitchPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        ui->frame->clear();
        return;
    }

    current = model->data(first, Qt::UserRole).value<Twitch::Modpack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    if (!current.authors.empty()) {
        auto authorToStr = [](Twitch::ModpackAuthor & author) {
            if(author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for(auto & author: current.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += tr(" by ") + authorStrs.join(", ");
    }

    ui->frame->setModText(text);
    ui->frame->setModDescription(current.description);
    suggestCurrent();
}

void TwitchPage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }
    if(current.broken)
    {
        dialog->setSuggestedPack();
    }

    dialog->setSuggestedPack(current.name, new InstanceImportTask(current.latestFile.downloadUrl));
    QString editedLogoName;
    editedLogoName = "twitch_" + current.logoName.section(".", 0, 0);
    model->getLogo(current.logoName, current.logoUrl, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}
