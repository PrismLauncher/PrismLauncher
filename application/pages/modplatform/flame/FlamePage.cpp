#include "FlamePage.h"
#include "ui_FlamePage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"
#include <InstanceImportTask.h>
#include "FlameModel.h"
#include <QKeyEvent>

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::FlamePage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &FlamePage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    model = new Flame::ListModel(this);
    ui->packView->setModel(model);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlamePage::onSelectionChanged);
}

FlamePage::~FlamePage()
{
    delete ui;
}

bool FlamePage::eventFilter(QObject* watched, QEvent* event)
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

bool FlamePage::shouldDisplay() const
{
    return true;
}

void FlamePage::openedImpl()
{
    suggestCurrent();
}

void FlamePage::triggerSearch()
{
    model->searchWithTerm(ui->searchEdit->text());
}

void FlamePage::onSelectionChanged(QModelIndex first, QModelIndex second)
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

    current = model->data(first, Qt::UserRole).value<Flame::Modpack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    if (!current.authors.empty()) {
        auto authorToStr = [](Flame::ModpackAuthor & author) {
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

void FlamePage::suggestCurrent()
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
    editedLogoName = "curseforge_" + current.logoName.section(".", 0, 0);
    model->getLogo(current.logoName, current.logoUrl, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}
