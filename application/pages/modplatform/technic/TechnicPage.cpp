/* Copyright 2013-2020 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TechnicPage.h"
#include "ui_TechnicPage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"
#include "TechnicModel.h"
#include <QKeyEvent>
#include "modplatform/technic/SingleZipPackInstallTask.h"
#include "modplatform/technic/SolderPackInstallTask.h"
#include "Json.h"

TechnicPage::TechnicPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::TechnicPage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &TechnicPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    model = new Technic::ListModel(this);
    ui->packView->setModel(model);
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TechnicPage::onSelectionChanged);
}

bool TechnicPage::eventFilter(QObject* watched, QEvent* event)
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

TechnicPage::~TechnicPage()
{
    delete ui;
}

bool TechnicPage::shouldDisplay() const
{
    return true;
}

void TechnicPage::openedImpl()
{
    dialog->setSuggestedPack();
}

void TechnicPage::triggerSearch() {
    model->searchWithTerm(ui->searchEdit->text());
}

void TechnicPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    if(!first.isValid())
    {
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        //ui->frame->clear();
        return;
    }

    current = model->data(first, Qt::UserRole).value<Technic::Modpack>();
    suggestCurrent();
}

void TechnicPage::suggestCurrent()
{
    if (!isOpened)
    {
        return;
    }
    if (current.broken)
    {
        dialog->setSuggestedPack();
        return;
    }

    QString editedLogoName;
    editedLogoName = "technic_" + current.logoName.section(".", 0, 0);
    model->getLogo(current.logoName, current.logoUrl, [this, editedLogoName](QString logo)
    {
        dialog->setSuggestedIconFromFile(logo, editedLogoName);
    });

    if (current.metadataLoaded)
    {
        metadataLoaded();
    }
    else
    {
        NetJob *netJob = new NetJob(QString("Technic::PackMeta(%1)").arg(current.name));
        std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
        QString slug = current.slug;
        netJob->addNetAction(Net::Download::makeByteArray(QString("https://api.technicpack.net/modpack/%1?build=multimc").arg(slug), response.get()));
        QObject::connect(netJob, &NetJob::succeeded, this, [this, response, slug]
        {
            if (current.slug != slug)
            {
                return;
            }
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            QJsonObject obj = doc.object();
            if(parse_error.error != QJsonParseError::NoError)
            {
                qWarning() << "Error while parsing JSON response from Technic at " << parse_error.offset << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            if (!obj.contains("url"))
            {
                qWarning() << "Json doesn't contain an url key";
                return;
            }
            QJsonValueRef url = obj["url"];
            if (url.isString())
            {
                current.url = url.toString();
            }
            else
            {
                if (!obj.contains("solder"))
                {
                    qWarning() << "Json doesn't contain a valid url or solder key";
                    return;
                }
                QJsonValueRef solderUrl = obj["solder"];
                if (solderUrl.isString())
                {
                    current.url = solderUrl.toString();
                    current.isSolder = true;
                }
                else
                {
                    qWarning() << "Json doesn't contain a valid url or solder key";
                    return;
                }
            }

            current.minecraftVersion = Json::ensureString(obj, "minecraft", QString(), "__placeholder__");
            current.websiteUrl = Json::ensureString(obj, "platformUrl", QString(), "__placeholder__");
            current.author = Json::ensureString(obj, "user", QString(), "__placeholder__");
            current.description = Json::ensureString(obj, "description", QString(), "__placeholder__");
            current.metadataLoaded = true;
            metadataLoaded();
        });
        netJob->start();
    }
}

// expects current.metadataLoaded to be true
void TechnicPage::metadataLoaded()
{
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        // This allows injecting HTML here.
        text = name;
    else
        // URL not properly escaped for inclusion in HTML. The name allows for injecting HTML.
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    if (!current.author.isEmpty()) {
        // This allows injecting HTML here
        text += tr(" by ") + current.author;
    }

    ui->frame->setModText(text);
    ui->frame->setModDescription(current.description);
    if (!current.isSolder)
    {
        dialog->setSuggestedPack(current.name, new Technic::SingleZipPackInstallTask(current.url, current.minecraftVersion));
    }
    else
    {
        while (current.url.endsWith('/')) current.url.chop(1);
        dialog->setSuggestedPack(current.name, new Technic::SolderPackInstallTask(current.url + "/modpack/" + current.slug, current.minecraftVersion));
    }
}
