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

#include "MultiMC.h"

#include "gui/dialogs/SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"

#include "logic/NagUtils.h"

#include "logic/java/JavaUtils.h"
#include "logic/java/JavaVersionList.h"
#include "logic/java/JavaChecker.h"

#include "logic/updater/UpdateChecker.h"

#include "logic/tools/BaseProfiler.h"

#include "logic/settings/SettingsObject.h"
#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

// FIXME: possibly move elsewhere
enum InstSortMode
{
	// Sort alphabetically by name.
	Sort_Name,
	// Sort by which instance was launched most recently.
	Sort_LastLaunch
};

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
	ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	ui->jsonEditorTextBox->setClearButtonEnabled(true);
#endif

	restoreGeometry(
		QByteArray::fromBase64(MMC->settings()->get("SettingsGeometry").toByteArray()));

	loadSettings(MMC->settings().get());
	updateCheckboxStuff();

	QObject::connect(MMC->updateChecker().get(), &UpdateChecker::channelListLoaded, this,
					 &SettingsDialog::refreshUpdateChannelList);

	if (MMC->updateChecker()->hasChannels())
	{
		refreshUpdateChannelList();
	}
	else
	{
		MMC->updateChecker()->updateChanList();
	}
	connect(ui->proxyGroup, SIGNAL(buttonClicked(int)), SLOT(proxyChanged(int)));
	ui->mceditLink->setOpenExternalLinks(true);
	ui->jvisualvmLink->setOpenExternalLinks(true);
	ui->jprofilerLink->setOpenExternalLinks(true);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}
void SettingsDialog::showEvent(QShowEvent *ev)
{
	QDialog::showEvent(ev);
}

void SettingsDialog::closeEvent(QCloseEvent *ev)
{
	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());

	QDialog::closeEvent(ev);
}

void SettingsDialog::updateCheckboxStuff()
{
	ui->windowWidthSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
	ui->windowHeightSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
	ui->proxyAddrBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
								 !ui->proxyDefaultBtn->isChecked());
	ui->proxyAuthBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
								 !ui->proxyDefaultBtn->isChecked());
}

void SettingsDialog::on_ftbLauncherBrowseBtn_clicked()
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

void SettingsDialog::on_ftbBrowseBtn_clicked()
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

void SettingsDialog::on_instDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Instance Directory"),
														ui->instDirTextBox->text());
	QString cooked_dir = NormalizePath(raw_dir);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!cooked_dir.isEmpty() && QDir(cooked_dir).exists())
	{
		ui->instDirTextBox->setText(cooked_dir);
	}
}
void SettingsDialog::on_iconsDirBrowseBtn_clicked()
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

void SettingsDialog::on_modsDirBrowseBtn_clicked()
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

void SettingsDialog::on_lwjglDirBrowseBtn_clicked()
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

void SettingsDialog::on_jsonEditorBrowseBtn_clicked()
{
	QString raw_file = QFileDialog::getOpenFileName(
		this, tr("JSON Editor"),
		ui->jsonEditorTextBox->text().isEmpty()
#if defined(Q_OS_LINUX)
				? QString("/usr/bin")
#else
			? QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()
#endif
			: ui->jsonEditorTextBox->text());
	QString cooked_file = NormalizePath(raw_file);

	if (cooked_file.isEmpty())
	{
		return;
	}

	// it has to exist and be an executable
	if (QFileInfo(cooked_file).exists() && QFileInfo(cooked_file).isExecutable())
	{
		ui->jsonEditorTextBox->setText(cooked_file);
	}
	else
	{
		QMessageBox::warning(this, tr("Invalid"),
							 tr("The file chosen does not seem to be an executable"));
	}
}

void SettingsDialog::on_maximizedCheckBox_clicked(bool checked)
{
	Q_UNUSED(checked);
	updateCheckboxStuff();
}

void SettingsDialog::on_buttonBox_accepted()
{
	applySettings(MMC->settings().get());

	// Apply proxy settings
	MMC->updateProxySettings();

	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());
}

void SettingsDialog::on_buttonBox_rejected()
{
	MMC->settings()->set("SettingsGeometry", saveGeometry().toBase64());
}

void SettingsDialog::proxyChanged(int)
{
	updateCheckboxStuff();
}

void SettingsDialog::refreshUpdateChannelList()
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

void SettingsDialog::updateChannelSelectionChanged(int index)
{
	refreshUpdateChannelDesc();
}

void SettingsDialog::refreshUpdateChannelDesc()
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

void SettingsDialog::applySettings(SettingsObject *s)
{
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
	case 0:
	default:
		s->set("IconTheme", "multimc");
		break;
	}
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

	// Editors
	QString jsonEditor = ui->jsonEditorTextBox->text();
	if (!jsonEditor.isEmpty() &&
		(!QFileInfo(jsonEditor).exists() || !QFileInfo(jsonEditor).isExecutable()))
	{
		QString found = QStandardPaths::findExecutable(jsonEditor);
		if (!found.isEmpty())
		{
			jsonEditor = found;
		}
	}
	s->set("JsonEditor", jsonEditor);

	// Console
	s->set("ShowConsole", ui->showConsoleCheck->isChecked());
	s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());

	// Window Size
	s->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
	s->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
	s->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());

	// Proxy
	QString proxyType = "None";
	if (ui->proxyDefaultBtn->isChecked())
		proxyType = "Default";
	else if (ui->proxyNoneBtn->isChecked())
		proxyType = "None";
	else if (ui->proxySOCKS5Btn->isChecked())
		proxyType = "SOCKS5";
	else if (ui->proxyHTTPBtn->isChecked())
		proxyType = "HTTP";

	s->set("ProxyType", proxyType);
	s->set("ProxyAddr", ui->proxyAddrEdit->text());
	s->set("ProxyPort", ui->proxyPortEdit->value());
	s->set("ProxyUser", ui->proxyUserEdit->text());
	s->set("ProxyPass", ui->proxyPassEdit->text());

	// Memory
	s->set("MinMemAlloc", ui->minMemSpinBox->value());
	s->set("MaxMemAlloc", ui->maxMemSpinBox->value());
	s->set("PermGen", ui->permGenSpinBox->value());

	// Java Settings
	s->set("JavaPath", ui->javaPathTextBox->text());
	s->set("JvmArgs", ui->jvmArgsTextBox->text());
	NagUtils::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());

	// Custom Commands
	s->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
	s->set("PostExitCommand", ui->postExitCmdTextBox->text());

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

	s->set("PostExitCommand", ui->postExitCmdTextBox->text());

	// Profilers
	s->set("JProfilerPath", ui->jprofilerPathEdit->text());
	s->set("JVisualVMPath", ui->jvisualvmPathEdit->text());
	s->set("MCEditPath", ui->mceditPathEdit->text());
}

void SettingsDialog::loadSettings(SettingsObject *s)
{
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
	else
	{
		ui->themeComboBox->setCurrentIndex(0);
	}
	// FTB
	ui->trackFtbBox->setChecked(s->get("TrackFTBInstances").toBool());
	ui->ftbLauncherBox->setText(s->get("FTBLauncherRoot").toString());
	ui->ftbBox->setText(s->get("FTBRoot").toString());

	// Folders
	ui->instDirTextBox->setText(s->get("InstanceDir").toString());
	ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
	ui->lwjglDirTextBox->setText(s->get("LWJGLDir").toString());
	ui->iconsDirTextBox->setText(s->get("IconsDir").toString());

	// Editors
	ui->jsonEditorTextBox->setText(s->get("JsonEditor").toString());

	// Console
	ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());

	// Window Size
	ui->maximizedCheckBox->setChecked(s->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(s->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(s->get("MinecraftWinHeight").toInt());

	// Memory
	ui->minMemSpinBox->setValue(s->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(s->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(s->get("PermGen").toInt());

	QString sortMode = s->get("InstSortMode").toString();

	if (sortMode == "LastLaunch")
	{
		ui->sortLastLaunchedBtn->setChecked(true);
	}
	else
	{
		ui->sortByNameBtn->setChecked(true);
	}

	// Proxy
	QString proxyType = s->get("ProxyType").toString();
	if (proxyType == "Default")
		ui->proxyDefaultBtn->setChecked(true);
	else if (proxyType == "None")
		ui->proxyNoneBtn->setChecked(true);
	else if (proxyType == "SOCKS5")
		ui->proxySOCKS5Btn->setChecked(true);
	else if (proxyType == "HTTP")
		ui->proxyHTTPBtn->setChecked(true);

	ui->proxyAddrEdit->setText(s->get("ProxyAddr").toString());
	ui->proxyPortEdit->setValue(s->get("ProxyPort").value<qint16>());
	ui->proxyUserEdit->setText(s->get("ProxyUser").toString());
	ui->proxyPassEdit->setText(s->get("ProxyPass").toString());

	// Java Settings
	ui->javaPathTextBox->setText(s->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(s->get("JvmArgs").toString());

	// Custom Commands
	ui->preLaunchCmdTextBox->setText(s->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(s->get("PostExitCommand").toString());

	// Profilers
	ui->jprofilerPathEdit->setText(s->get("JProfilerPath").toString());
	ui->jvisualvmPathEdit->setText(s->get("JVisualVMPath").toString());
	ui->mceditPathEdit->setText(s->get("MCEditPath").toString());
}

void SettingsDialog::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void SettingsDialog::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}

void SettingsDialog::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void SettingsDialog::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform + "\n";
		text += "Java version reported: " + result.javaVersion;
		QMessageBox::information(this, tr("Java test success"), text);
	}
	else
	{
		QMessageBox::warning(
			this, tr("Java test failure"),
			tr("The specified java binary didn't work. You should use the auto-detect feature, "
			   "or set the path to the java executable."));
	}
}

void SettingsDialog::on_jprofilerPathBtn_clicked()
{
	QString raw_dir = ui->jprofilerPathEdit->text();
	QString error;
	do
	{
		raw_dir = QFileDialog::getExistingDirectory(this, tr("JProfiler Directory"), raw_dir);
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->profilers()["jprofiler"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking JProfiler install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->jprofilerPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}
void SettingsDialog::on_jprofilerCheckBtn_clicked()
{
	QString error;
	if (!MMC->profilers()["jprofiler"]->check(ui->jprofilerPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking JProfiler install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("JProfiler setup seems to be OK"));
	}
}

void SettingsDialog::on_jvisualvmPathBtn_clicked()
{
	QString raw_dir = ui->jvisualvmPathEdit->text();
	QString error;
	do
	{
		raw_dir = QFileDialog::getOpenFileName(this, tr("JVisualVM Executable"), raw_dir);
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->profilers()["jvisualvm"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking JVisualVM install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->jvisualvmPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}
void SettingsDialog::on_jvisualvmCheckBtn_clicked()
{
	QString error;
	if (!MMC->profilers()["jvisualvm"]->check(ui->jvisualvmPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking JVisualVM install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("JVisualVM setup seems to be OK"));
	}
}

void SettingsDialog::on_mceditPathBtn_clicked()
{
	QString raw_dir = ui->mceditPathEdit->text();
	QString error;
	do
	{
#ifdef Q_OS_OSX
#warning stuff
		raw_dir = QFileDialog::getOpenFileName(this, tr("MCEdit Application"), raw_dir);
#else
		raw_dir = QFileDialog::getExistingDirectory(this, tr("MCEdit Directory"), raw_dir);
#endif
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->tools()["mcedit"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking MCEdit install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->mceditPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}

void SettingsDialog::on_mceditCheckBtn_clicked()
{
	QString error;
	if (!MMC->tools()["mcedit"]->check(ui->mceditPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking MCEdit install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("MCEdit setup seems to be OK"));
	}
}
