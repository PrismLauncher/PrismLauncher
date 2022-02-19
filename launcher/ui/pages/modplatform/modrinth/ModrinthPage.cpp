#include "ModrinthPage.h"
#include "ui_ModrinthPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "Json.h"
#include "ui/dialogs/ModDownloadDialog.h"
#include "InstanceImportTask.h"
#include "ModrinthModel.h"
#include "ModDownloadTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

ModrinthPage::ModrinthPage(ModDownloadDialog *dialog, BaseInstance *instance)
    : QWidget(dialog), m_instance(instance), ui(new Ui::ModrinthPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &ModrinthPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    listModel = new Modrinth::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    // index is used to set the sorting with the modrinth api
    ui->sortByBox->addItem(tr("Sort by Relevence"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));
    ui->sortByBox->addItem(tr("Sort by Follows"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by newest"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthPage::onVersionSelectionChanged);
}

ModrinthPage::~ModrinthPage()
{
    delete ui;
}

bool ModrinthPage::eventFilter(QObject* watched, QEvent* event)
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

bool ModrinthPage::shouldDisplay() const
{
    return true;
}

void ModrinthPage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void ModrinthPage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void ModrinthPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedMod();
        }
        return;
    }

    current = listModel->data(first, Qt::UserRole).value<Modrinth::IndexedPack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    text += "<br>"+ tr(" by ") + "<a href=\""+current.author.url+"\">"+current.author.name+"</a><br><br>";
    ui->packDescription->setHtml(text + current.description);

    if (!current.versionsLoaded)
    {
        qDebug() << "Loading Modrinth mod versions";
        auto netJob = new NetJob(QString("Modrinth::ModVersions(%1)").arg(current.name), APPLICATION->network());
        auto response = new QByteArray();
        QString addonId = current.addonId;
        netJob->addNetAction(Net::Download::makeByteArray(QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId), response));

        QObject::connect(netJob, &NetJob::succeeded, this, [this, response]
        {
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if(parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            QJsonArray arr = doc.array();
            try
            {
                Modrinth::loadIndexedPackVersions(current, arr, APPLICATION->network(), m_instance);
            }
            catch(const JSONValidationError &e)
            {
                qDebug() << *response;
                qWarning() << "Error while reading Modrinth mod version: " << e.cause();
            }
            auto packProfile = ((MinecraftInstance *)m_instance)->getPackProfile();
            QString mcVersion =  packProfile->getComponentVersion("net.minecraft");
            QString loaderString = (packProfile->getComponentVersion("net.minecraftforge").isEmpty()) ? "fabric" : "forge";
            for(int i = 0; i < current.versions.size(); i++) {
                auto version = current.versions[i];
                if(!version.mcVersion.contains(mcVersion) || !version.loaders.contains(loaderString)){
                    continue;
                }
                ui->versionSelectionBox->addItem(version.version, QVariant(i));
            }
            if(ui->versionSelectionBox->count() == 0){
                ui->versionSelectionBox->addItem(tr("No Valid Version found !"), QVariant(-1));
            }

            suggestCurrent();
        });
        QObject::connect(netJob, &NetJob::finished, this, [response, netJob]{
            netJob->deleteLater();
            delete response;
        });
        netJob->start();
    }
    else
    {
        for(int i = 0; i < current.versions.size(); i++) {
            ui->versionSelectionBox->addItem(current.versions[i].version, QVariant(i));
        }
        if(ui->versionSelectionBox->count() == 0){
            ui->versionSelectionBox->addItem(tr("No Valid Version found !"), QVariant(-1));
        }
        suggestCurrent();
    }
}

void ModrinthPage::suggestCurrent()
{
    if(!isOpened)
    {
        return;
    }

    if (selectedVersion == -1)
    {
        dialog->setSuggestedMod();
        return;
    }
    auto version = current.versions[selectedVersion];
    dialog->setSuggestedMod(current.name, new ModDownloadTask(version.downloadUrl, version.fileName , dialog->mods));
}

void ModrinthPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = -1;
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toInt();
    suggestCurrent();
}
