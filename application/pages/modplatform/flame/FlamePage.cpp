#include "FlamePage.h"
#include "ui_FlamePage.h"

#include "MultiMC.h"
#include <Json.h>
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
    listModel = new Flame::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->setMaxVisibleItems(10);

    // index is used to set the sorting with the curseforge api
    ui->sortByBox->addItem(tr("Sort by featured"));
    ui->sortByBox->addItem(tr("Sort by popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by name"));
    ui->sortByBox->addItem(tr("Sort by author"));
    ui->sortByBox->addItem(tr("Sort by total downloads"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlamePage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlamePage::onVersionSelectionChanged);
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
    triggerSearch();
}

void FlamePage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void FlamePage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }

    current = listModel->data(first, Qt::UserRole).value<Flame::IndexedPack>();
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
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }
    text += "<br><br>";

    ui->packDescription->setHtml(text + current.description);

    if (current.versionsLoaded == false)
    {
        qDebug() << "Loading flame modpack versions";
        NetJob *netJob = new NetJob(QString("Flame::PackVersions(%1)").arg(current.name));
        std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
        int addonId = current.addonId;
        netJob->addNetAction(Net::Download::makeByteArray(QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(addonId), response.get()));
        
        QObject::connect(netJob, &NetJob::succeeded, this, [this, response]
        {
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if(parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            QJsonArray arr = doc.array();
            try
            {
                Flame::loadIndexedPackVersions(current, arr);
            }
            catch(const JSONValidationError &e)
            {
                qDebug() << *response;
                qWarning() << "Error while reading flame modpack version: " << e.cause();
            }

            for(auto version : current.versions) {
                ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
            }

            suggestCurrent();
        });
        netJob->start();
    }
    else
    {
        for(auto version : current.versions) {
            ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }
}

void FlamePage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    dialog->setSuggestedPack(current.name, new InstanceImportTask(selectedVersion));
    QString editedLogoName;
    editedLogoName = "curseforge_" + current.logoName.section(".", 0, 0);
    listModel->getLogo(current.logoName, current.logoUrl, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });
}

void FlamePage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toString();
    suggestCurrent();
}
