/* Copyright 2013 MultiMC Contributors
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

#include "logindialog.h"
#include "ui_logindialog.h"
#include "keyring.h"
#include "gui/platform.h"
#include "MultiMC.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "logic/net/HttpMetaCache.h"
#include <logger/QsLog.h>

LoginDialog::LoginDialog(QWidget *parent, const QString& loginErrMsg) :
	QDialog(parent),
	ui(new Ui::LoginDialog)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	
	//: Use offline mode one time
	offlineButton = new QPushButton(tr("Offline Once"));
	
	ui->loginButtonBox->addButton(offlineButton, QDialogButtonBox::ActionRole);
	
	blockToggles = false;
	isOnline_ = true;
	onlineForced = false;
	
	//: The username during login (placeholder)
	ui->usernameTextBox->lineEdit()->setPlaceholderText(tr("Name"));
	
	connect(ui->usernameTextBox, SIGNAL(currentTextChanged(QString)), this, SLOT(userTextChanged(QString)));
	connect(ui->forgetButton, SIGNAL(clicked(bool)), this, SLOT(forgetCurrentUser()));
	connect(offlineButton, SIGNAL(clicked(bool)), this, SLOT(launchOffline()));
	
	if (loginErrMsg.isEmpty())
		ui->loginErrorLabel->setVisible(false);
	else
	{
		ui->loginErrorLabel->setVisible(true);
		ui->loginErrorLabel->setText(QString("<span style=\" color:#ff0000;\">%1</span>").
								   arg(loginErrMsg));
	}
	
	ui->lblFace->setVisible(false);

	resize(minimumSizeHint());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	Keyring * k = Keyring::instance();
	QStringList accounts = k->getStoredAccounts("minecraft");
	ui->usernameTextBox->addItems(accounts);
	
	// TODO: restore last selected account here, if applicable
	
	int index = ui->usernameTextBox->currentIndex();
	if(index != -1)
	{
		ui->passwordTextBox->setFocus(Qt::OtherFocusReason);
	}
	
	connect(ui->rememberUsernameCheckbox,SIGNAL(toggled(bool)), SLOT(usernameToggled(bool)));
	connect(ui->rememberPasswordCheckbox,SIGNAL(toggled(bool)), SLOT(passwordToggled(bool)));
}

LoginDialog::~LoginDialog()
{
	delete offlineButton;
	delete ui;
}

QString LoginDialog::getUsername() const
{
	return ui->usernameTextBox->currentText();
}

QString LoginDialog::getPassword() const
{
	return ui->passwordTextBox->text();
}

void LoginDialog::forgetCurrentUser()
{
	Keyring * k = Keyring::instance();
	QString acct = ui->usernameTextBox->currentText();
	k->removeStoredAccount("minecraft", acct);
	ui->passwordTextBox->clear();
	int index = ui->usernameTextBox->findText(acct);
	if(index != -1)
		ui->usernameTextBox->removeItem(index);
	if(!ui->usernameTextBox->count())
	{
		blockToggles = true;
		ui->rememberUsernameCheckbox->setChecked(false);
		ui->rememberPasswordCheckbox->setChecked(false);
		blockToggles = false;
	}
}

void LoginDialog::passwordToggled ( bool state )
{
	// if toggled off
	if(blockToggles)
		return;
	blockToggles = true;
	if(!state)
	{
		QLOG_DEBUG() << "password disabled";
	}
	else
	{
		if(!ui->rememberUsernameCheckbox->isChecked())
		{
			ui->rememberUsernameCheckbox->setChecked(true);
		}
		QLOG_DEBUG() << "password enabled";
	}
	blockToggles = false;
}

void LoginDialog::usernameToggled ( bool state )
{
	// if toggled off
	if(blockToggles)
		return;
	blockToggles = true;
	if(!state)
	{
		if(ui->rememberPasswordCheckbox->isChecked())
		{
			ui->rememberPasswordCheckbox->setChecked(false);
		}
		QLOG_DEBUG() << "username disabled";
	}
	else
	{
		QLOG_DEBUG() << "username enabled";
	}
	blockToggles = false;
}


void LoginDialog::userTextChanged ( const QString& user )
{
	blockToggles = true;
	Keyring * k = Keyring::instance();
	QStringList sl = k->getStoredAccounts("minecraft");
	bool gotFace = false;

	if(sl.contains(user))
	{
		ui->rememberUsernameCheckbox->setChecked(true);
		QString passwd = k->getPassword("minecraft",user);
		ui->rememberPasswordCheckbox->setChecked(!passwd.isEmpty());
		ui->passwordTextBox->setText(passwd);

		QByteArray data;
		{
			auto filename = MMC->metacache()->resolveEntry("skins", "skins.json")->getFullPath();
			QFile listFile(filename);
			if(!listFile.open(QIODevice::ReadOnly))
				return;
			data = listFile.readAll();
		}

		QJsonParseError jsonError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
		QJsonObject root = jsonDoc.object();
		QJsonObject mappings = root.value("mappings").toObject();

		if(!mappings[user].isUndefined())
		{
			QJsonArray usernames = mappings.value(user).toArray();
			if(!usernames.isEmpty())
			{
				QString mapped_username = usernames[0].toString();

				if(!mapped_username.isEmpty())
				{
					QFile fskin(MMC->metacache()->resolveEntry("skins", mapped_username + ".png")->getFullPath());
					if(fskin.exists())
					{
						QPixmap skin(MMC->metacache()->resolveEntry("skins", mapped_username + ".png")->getFullPath());
						QPixmap face = skin.copy(8, 8, 8, 8).scaled(48, 48, Qt::KeepAspectRatio);

						ui->lblFace->setPixmap(face);
						gotFace = true;
					}
				}
			}
		}
	}

	if(gotFace) ui->lblFace->setVisible(true);
	else ui->lblFace->setVisible(false);
	blockToggles = false;
}


void LoginDialog::accept()
{
	bool saveName = ui->rememberUsernameCheckbox->isChecked();
	bool savePass = ui->rememberPasswordCheckbox->isChecked();
	Keyring * k = Keyring::instance();
	if(saveName)
	{
		if(savePass)
		{
			k->storePassword("minecraft",getUsername(),getPassword());
		}
		else
		{
			k->storePassword("minecraft",getUsername(),QString());
		}
	}
	else
	{
		QString acct = ui->usernameTextBox->currentText();
		k->removeStoredAccount("minecraft", acct);
	}
	QDialog::accept();
}

void LoginDialog::launchOffline() 
{
	isOnline_ = false;
	QDialog::accept();
}

void LoginDialog::forceOnline()
{
	onlineForced = true;
	offlineButton->setEnabled(false);
}
