/* Copyright 2013-2021 MultiMC Contributors
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

#include "ProfileSetupDialog.h"
#include "ui_ProfileSetupDialog.h"

#include <QPushButton>
#include <QAction>
#include <QRegExpValidator>
#include <QJsonDocument>
#include <QDebug>

#include "ui/dialogs/ProgressDialog.h"

#include <Application.h>
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"


ProfileSetupDialog::ProfileSetupDialog(MinecraftAccountPtr accountToSetup, QWidget *parent)
    : QDialog(parent), m_accountToSetup(accountToSetup), ui(new Ui::ProfileSetupDialog)
{
    ui->setupUi(this);
    ui->errorLabel->setVisible(false);

    goodIcon = APPLICATION->getThemedIcon("status-good");
    yellowIcon = APPLICATION->getThemedIcon("status-yellow");
    badIcon = APPLICATION->getThemedIcon("status-bad");

    QRegExp permittedNames("[a-zA-Z0-9_]{3,16}");
    auto nameEdit = ui->nameEdit;
    nameEdit->setValidator(new QRegExpValidator(permittedNames));
    nameEdit->setClearButtonEnabled(true);
    validityAction = nameEdit->addAction(yellowIcon, QLineEdit::LeadingPosition);
    connect(nameEdit, &QLineEdit::textEdited, this, &ProfileSetupDialog::nameEdited);

    checkStartTimer.setSingleShot(true);
    connect(&checkStartTimer, &QTimer::timeout, this, &ProfileSetupDialog::startCheck);

    setNameStatus(NameStatus::NotSet, QString());
}

ProfileSetupDialog::~ProfileSetupDialog()
{
    delete ui;
}

void ProfileSetupDialog::on_buttonBox_accepted()
{
    setupProfile(currentCheck);
}

void ProfileSetupDialog::on_buttonBox_rejected()
{
    reject();
}

void ProfileSetupDialog::setNameStatus(ProfileSetupDialog::NameStatus status, QString errorString = QString())
{
    nameStatus = status;
    auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    switch(nameStatus)
    {
        case NameStatus::Available: {
            validityAction->setIcon(goodIcon);
            okButton->setEnabled(true);
        }
        break;
        case NameStatus::NotSet:
        case NameStatus::Pending:
            validityAction->setIcon(yellowIcon);
            okButton->setEnabled(false);
            break;
        case NameStatus::Exists:
        case NameStatus::Error:
            validityAction->setIcon(badIcon);
            okButton->setEnabled(false);
            break;
    }
    if(!errorString.isEmpty()) {
        ui->errorLabel->setText(errorString);
        ui->errorLabel->setVisible(true);
    }
    else {
        ui->errorLabel->setVisible(false);
    }
}

void ProfileSetupDialog::nameEdited(const QString& name)
{
    if(!ui->nameEdit->hasAcceptableInput()) {
        setNameStatus(NameStatus::NotSet, tr("Name is too short - must be between 3 and 16 characters long."));
        return;
    }
    scheduleCheck(name);
}

void ProfileSetupDialog::scheduleCheck(const QString& name) {
    queuedCheck = name;
    setNameStatus(NameStatus::Pending);
    checkStartTimer.start(1000);
}

void ProfileSetupDialog::startCheck() {
    if(isChecking) {
        return;
    }
    if(queuedCheck.isNull()) {
        return;
    }
    checkName(queuedCheck);
}


void ProfileSetupDialog::checkName(const QString &name) {
    if(isChecking) {
        return;
    }

    currentCheck = name;
    isChecking = true;

    auto token = m_accountToSetup->accessToken();

    auto url = QString("https://api.minecraftservices.com/minecraft/profile/name/%1/available").arg(name);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &ProfileSetupDialog::checkFinished);
    requestor->get(request);
}

void ProfileSetupDialog::checkFinished(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if(error == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(data);
        auto root = doc.object();
        auto statusValue = root.value("status").toString("INVALID");
        if(statusValue == "AVAILABLE") {
            setNameStatus(NameStatus::Available);
        }
        else if (statusValue == "DUPLICATE") {
            setNameStatus(NameStatus::Exists, tr("Minecraft profile with name %1 already exists.").arg(currentCheck));
        }
        else if (statusValue == "NOT_ALLOWED") {
            setNameStatus(NameStatus::Exists, tr("The name %1 is not allowed.").arg(currentCheck));
        }
        else {
            setNameStatus(NameStatus::Error, tr("Unhandled profile name status: %1").arg(statusValue));
        }
    }
    else {
        setNameStatus(NameStatus::Error, tr("Failed to check name availability."));
    }
    isChecking = false;
}

void ProfileSetupDialog::setupProfile(const QString &profileName) {
    if(isWorking) {
        return;
    }

    auto token = m_accountToSetup->accessToken();

    auto url = QString("https://api.minecraftservices.com/minecraft/profile");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

    QString payloadTemplate("{\"profileName\":\"%1\"}");
    auto data = payloadTemplate.arg(profileName).toUtf8();

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &ProfileSetupDialog::setupProfileFinished);
    requestor->post(request, data);
    isWorking = true;

    auto button = ui->buttonBox->button(QDialogButtonBox::Cancel);
    button->setEnabled(false);
}

namespace {

struct MojangError{
    static MojangError fromJSON(QByteArray data) {
        MojangError out;
        out.error = QString::fromUtf8(data);
        auto doc = QJsonDocument::fromJson(data, &out.parseError);
        auto object = doc.object();

        out.fullyParsed = true;
        out.fullyParsed &= Parsers::getString(object.value("path"), out.path);
        out.fullyParsed &= Parsers::getString(object.value("error"), out.error);
        out.fullyParsed &= Parsers::getString(object.value("errorMessage"), out.errorMessage);

        return out;
    }

    QString rawError;
    QJsonParseError parseError;
    bool fullyParsed;

    QString path;
    QString error;
    QString errorMessage;
};

}

void ProfileSetupDialog::setupProfileFinished(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    isWorking = false;
    if(error == QNetworkReply::NoError) {
        /*
         * data contains the profile in the response
         * ... we could parse it and update the account, but let's just return back to the normal login flow instead...
         */
        accept();
    }
    else {
        auto parsedError = MojangError::fromJSON(data);
        ui->errorLabel->setVisible(true);
        ui->errorLabel->setText(tr("The server returned the following error:") + "\n\n" + parsedError.errorMessage);
        qDebug() << parsedError.rawError;
        auto button = ui->buttonBox->button(QDialogButtonBox::Cancel);
        button->setEnabled(true);
    }
}
