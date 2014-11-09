/* Copyright 2013-2014 MultiMC Contributors
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

#include "MultiMCPage.h"
#include "ui_MultiMCPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include <pathutils.h>

#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"
#include <gui/ColumnResizer.h>

#include "logic/NagUtils.h"

#include "logic/java/JavaUtils.h"
#include "logic/java/JavaVersionList.h"
#include "logic/java/JavaChecker.h"

#include "logic/updater/UpdateChecker.h"

#include "logic/tools/BaseProfiler.h"

#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

// FIXME: possibly move elsewhere
enum InstSortMode
{
	// Sort alphabetically by name.
	Sort_Name,
	// Sort by which instance was launched most recently.
	Sort_LastLaunch
};

MultiMCPage::MultiMCPage(QWidget *parent) : QWidget(parent), ui(new Ui::MultiMCPage)
{
	ui->setupUi(this);
	ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
	ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

	auto resizer = new ColumnResizer(this);
	resizer->addWidgetsFromLayout(ui->groupBox->layout(), 1);
	resizer->addWidgetsFromLayout(ui->foldersBox->layout(), 1);

	loadSettings();

	QObject::connect(MMC->updateChecker().get(), &UpdateChecker::channelListLoaded, this,
					 &MultiMCPage::refreshUpdateChannelList);

	if (MMC->updateChecker()->hasChannels())
	{
		refreshUpdateChannelList();
	}
	else
	{
		MMC->updateChecker()->updateChanList(false);
	}
}

MultiMCPage::~MultiMCPage()
{
	delete ui;
}

bool MultiMCPage::apply()
{
	applySettings();
	return true;
}

void MultiMCPage::on_ftbLauncherBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("FTB Launcher Directory"),
														ui->ftbLauncherBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->ftbLauncherBox->setText(cooked_dir);
	}
}
void MultiMCPage::on_ftbBrowseBtn_clicked()
{
	QString raw_dir =
		QFileDialog::getExistingDirectory(this, tr("FTB Directory"), ui->ftbBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->ftbBox->setText(cooked_dir);
	}
}

void MultiMCPage::on_instDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Instance Directory"),
                                                        ui->instDirTextBox->text());
    QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		if (checkProblemticPathJava(QDir(cooked_dir)))
		{
			QMessageBox warning;
			warning.setText(tr("You're trying to specify an instance folder which\'s path "
							   "contains at least one \'!\'. "
							   "Java is known to cause problems if that is the case, your "
							   "instances (probably) won't start!"));
			warning.setInformativeText(
				tr("Do you really want to use this path? "
				   "Selecting \"No\" will close this and not alter your instance path."));
			warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			int result = warning.exec();
			if (result == QMessageBox::Yes)
			{
				ui->instDirTextBox->setText(cooked_dir);
			}
		}
		else
		{
			ui->instDirTextBox->setText(cooked_dir);
		}
	}
}

void MultiMCPage::on_iconsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Icons Directory"),
														ui->iconsDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->iconsDirTextBox->setText(cooked_dir);
	}
}
void MultiMCPage::on_modsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Mods Directory"),
														ui->modsDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->modsDirTextBox->setText(cooked_dir);
	}
}
void MultiMCPage::on_lwjglDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("LWJGL Directory"),
														ui->lwjglDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->lwjglDirTextBox->setText(cooked_dir);
	}
}

void MultiMCPage::refreshUpdateChannelList()
{
	// Stop listening for selection changes. It's going to change a lot while we update it and
	// we don't need to update the
	// description label constantly.
	QObject::disconnect(ui->updateChannelComboBox, SIGNAL(currentIndexChanged(int)), this,
						SLOT(updateChannelSelectionChanged(int)));

	QList<UpdateChecker::ChannelListEntry> channelList = MMC->updateChecker()->getChannelList();
	ui->updateChannelComboBox->clear();
	int selection = -1;
	for (int i = 0; i < channelList.count(); i++)
	{
		UpdateChecker::ChannelListEntry entry = channelList.at(i);

		// When it comes to selection, we'll rely on the indexes of a channel entry being the
		// same in the
		// combo box as it is in the update checker's channel list.
		// This probably isn't very safe, but the channel list doesn't change often enough (or
		// at all) for
		// this to be a big deal. Hope it doesn't break...
		ui->updateChannelComboBox->addItem(entry.name);

		// If the update channel we just added was the selected one, set the current index in
		// the combo box to it.
		if (entry.id == m_currentUpdateChannel)
		{
			QLOG_DEBUG() << "Selected index" << i << "channel id" << m_currentUpdateChannel;
			selection = i;
		}
	}

	ui->updateChannelComboBox->setCurrentIndex(selection);

	// Start listening for selection changes again and update the description label.
	QObject::connect(ui->updateChannelComboBox, SIGNAL(currentIndexChanged(int)), this,
					 SLOT(updateChannelSelectionChanged(int)));
	refreshUpdateChannelDesc();

	// Now that we've updated the channel list, we can enable the combo box.
	// It starts off disabled so that if the channel list hasn't been loaded, it will be
	// disabled.
	ui->updateChannelComboBox->setEnabled(true);
}

void MultiMCPage::updateChannelSelectionChanged(int index)
{
	refreshUpdateChannelDesc();
}

void MultiMCPage::refreshUpdateChannelDesc()
{
	// Get the channel list.
	QList<UpdateChecker::ChannelListEntry> channelList = MMC->updateChecker()->getChannelList();
	int selectedIndex = ui->updateChannelComboBox->currentIndex();
	if (selectedIndex < 0)
	{
		return;
	}
	if (selectedIndex < channelList.count())
	{
		// Find the channel list entry with the given index.
		UpdateChecker::ChannelListEntry selected = channelList.at(selectedIndex);

		// Set the description text.
		ui->updateChannelDescLabel->setText(selected.description);

		// Set the currently selected channel ID.
		m_currentUpdateChannel = selected.id;
	}
}

void MultiMCPage::applySettings()
{
	auto s = MMC->settings();
	// Language
	s->set("Language",
		   ui->languageBox->itemData(ui->languageBox->currentIndex()).toLocale().bcp47Name());

	if (ui->resetNotificationsBtn->isChecked())
	{
		s->set("ShownNotifications", QString());
	}

	// Updates
	s->set("AutoUpdate", ui->autoUpdateCheckBox->isChecked());
	s->set("UpdateChannel", m_currentUpdateChannel);
	//FIXME: make generic
	switch (ui->themeComboBox->currentIndex())
	{
	case 1:
		s->set("IconTheme", "pe_dark");
		break;
	case 2:
		s->set("IconTheme", "pe_light");
		break;
	case 3:
		s->set("IconTheme", "pe_blue");
		break;
	case 4:
		s->set("IconTheme", "pe_colored");
		break;
	case 5:
		s->set("IconTheme", "OSX");
		break;
	case 6:
		s->set("IconTheme", "iOS");
		break;
	case 0:
	default:
		s->set("IconTheme", "multimc");
		break;
	}

	// Console settings
	QString consoleFontFamily = ui->consoleFont->currentFont().family();
	s->set("ConsoleFont", consoleFontFamily);

	// FTB
	s->set("TrackFTBInstances", ui->trackFtbBox->isChecked());
	s->set("FTBLauncherRoot", ui->ftbLauncherBox->text());
	s->set("FTBRoot", ui->ftbBox->text());

	// Folders
	// TODO: Offer to move instances to new instance folder.
	s->set("InstanceDir", ui->instDirTextBox->text());
	s->set("CentralModsDir", ui->modsDirTextBox->text());
	s->set("LWJGLDir", ui->lwjglDirTextBox->text());
	s->set("IconsDir", ui->iconsDirTextBox->text());

	auto sortMode = (InstSortMode)ui->sortingModeGroup->checkedId();
	switch (sortMode)
	{
	case Sort_LastLaunch:
		s->set("InstSortMode", "LastLaunch");
		break;
	case Sort_Name:
	default:
		s->set("InstSortMode", "Name");
		break;
	}
}
void MultiMCPage::loadSettings()
{
	auto s = MMC->settings();
	// Language
	ui->languageBox->clear();
	ui->languageBox->addItem(tr("English"), QLocale(QLocale::English));
	foreach(const QString & lang, QDir(MMC->staticData() + "/translations")
									  .entryList(QStringList() << "*.qm", QDir::Files))
	{
		QLocale locale(lang.section(QRegExp("[_\\.]"), 1));
		ui->languageBox->addItem(QLocale::languageToString(locale.language()), locale);
	}
	ui->languageBox->setCurrentIndex(
		ui->languageBox->findData(QLocale(s->get("Language").toString())));

	// Updates
	ui->autoUpdateCheckBox->setChecked(s->get("AutoUpdate").toBool());
	m_currentUpdateChannel = s->get("UpdateChannel").toString();
	//FIXME: make generic
	auto theme = s->get("IconTheme").toString();
	if (theme == "pe_dark")
	{
		ui->themeComboBox->setCurrentIndex(1);
	}
	else if (theme == "pe_light")
	{
		ui->themeComboBox->setCurrentIndex(2);
	}
	else if (theme == "pe_blue")
	{
		ui->themeComboBox->setCurrentIndex(3);
	}
	else if (theme == "pe_colored")
	{
		ui->themeComboBox->setCurrentIndex(4);
	}
	else if (theme == "OSX")
	{
		ui->themeComboBox->setCurrentIndex(5);
	}
	else if (theme == "iOS")
	{
		ui->themeComboBox->setCurrentIndex(6);
	}
	else
	{
		ui->themeComboBox->setCurrentIndex(0);
	}

	// Console settings
	QString fontFamily = MMC->settings()->get("ConsoleFont").toString();
	QFont consoleFont(fontFamily);
	ui->consoleFont->setCurrentFont(consoleFont);

	// FTB
	ui->trackFtbBox->setChecked(s->get("TrackFTBInstances").toBool());
	ui->ftbLauncherBox->setText(s->get("FTBLauncherRoot").toString());
	ui->ftbBox->setText(s->get("FTBRoot").toString());

	// Folders
	ui->instDirTextBox->setText(s->get("InstanceDir").toString());
	ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
	ui->lwjglDirTextBox->setText(s->get("LWJGLDir").toString());
	ui->iconsDirTextBox->setText(s->get("IconsDir").toString());

	QString sortMode = s->get("InstSortMode").toString();

	if (sortMode == "LastLaunch")
	{
		ui->sortLastLaunchedBtn->setChecked(true);
	}
	else
	{
		ui->sortByNameBtn->setChecked(true);
	}
}
