#include "FlameModPage.h"
#include "ui_FlameModPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "Json.h"
#include "ui/dialogs/ModDownloadDialog.h"
#include "InstanceImportTask.h"
#include "FlameModModel.h"
#include "ModDownloadTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

FlameModPage::FlameModPage(ModDownloadDialog *dialog, BaseInstance *instance)
    : QWidget(dialog), m_instance(instance), ui(new Ui::FlameModPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &FlameModPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    listModel = new FlameMod::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    // index is used to set the sorting with the flame api
    ui->sortByBox->addItem(tr("Sort by Featured"));
    ui->sortByBox->addItem(tr("Sort by Popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by Name"));
    ui->sortByBox->addItem(tr("Sort by Author"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
}

FlameModPage::~FlameModPage()
{
    delete ui;
}

bool FlameModPage::eventFilter(QObject* watched, QEvent* event)
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

bool FlameModPage::shouldDisplay() const
{
    return true;
}

void FlameModPage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void FlameModPage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void FlameModPage::onSelectionChanged(QModelIndex first, QModelIndex second)
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

    current = listModel->data(first, Qt::UserRole).value<FlameMod::IndexedPack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    if (!current.authors.empty()) {
        auto authorToStr = [](FlameMod::ModpackAuthor & author) {
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

    if (!current.versionsLoaded)
    {
        qDebug() << "Loading flame mod versions";
        auto netJob = new NetJob(QString("Flame::ModVersions(%1)").arg(current.name), APPLICATION->network());
        auto response = new QByteArray();
        int addonId = current.addonId;
        netJob->addNetAction(Net::Download::makeByteArray(QString("https://addons-ecs.forgesvc.net/api/v2/addon/%1/files").arg(addonId), response));

        QObject::connect(netJob, &NetJob::succeeded, this, [this, response]
        {
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if(parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from Flame at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            QJsonArray arr = doc.array();
            try
            {
                FlameMod::loadIndexedPackVersions(current, arr, APPLICATION->network(), m_instance);
            }
            catch(const JSONValidationError &e)
            {
                qDebug() << *response;
                qWarning() << "Error while reading Flame mod version: " << e.cause();
            }
            auto packProfile = ((MinecraftInstance *)m_instance)->getPackProfile();
            QString mcVersion =  packProfile->getComponentVersion("net.minecraft");
            QString loaderString = (packProfile->getComponentVersion("net.minecraftforge").isEmpty()) ? "fabric" : "forge";
            for(int i = 0; i < current.versions.size(); i++) {
                auto version = current.versions[i];
                if(!version.mcVersion.contains(mcVersion)){
                    continue;
                }
                ui->versionSelectionBox->addItem(version.version, QVariant(i));
            }
            if(ui->versionSelectionBox->count() == 0){
                ui->versionSelectionBox->addItem(tr("No Valid Version found!"), QVariant(-1));
            }
            suggestCurrent();
        });
        QObject::connect(netJob, &NetJob::finished, this, [response, netJob]
        {
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
            ui->versionSelectionBox->addItem(tr("No Valid Version found!"), QVariant(-1));
        }
        suggestCurrent();
    }
}

void FlameModPage::suggestCurrent()
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

void FlameModPage::onVersionSelectionChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = -1;
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toInt();
    suggestCurrent();
}
