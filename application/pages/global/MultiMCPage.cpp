/* Copyright 2013-2018 MultiMC Contributors
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
#include <QTextCharFormat>

#include "updater/UpdateChecker.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
#include "MultiMC.h"
#include "BuildConfig.h"
#include "themes/ITheme.h"

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
    auto origForeground = ui->fontPreview->palette().color(ui->fontPreview->foregroundRole());
    auto origBackground = ui->fontPreview->palette().color(ui->fontPreview->backgroundRole());
    m_colors.reset(new LogColorCache(origForeground, origBackground));

    ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
    ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

    defaultFormat = new QTextCharFormat(ui->fontPreview->currentCharFormat());

    m_languageModel = MMC->translations();
    loadSettings();

    if(BuildConfig.UPDATER_ENABLED)
    {
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
    else
    {
        ui->updateSettingsBox->setHidden(true);
    }
    // Analytics
    if(BuildConfig.ANALYTICS_ID.isEmpty())
    {
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->analyticsTab));
    }
    connect(ui->fontSizeBox, SIGNAL(valueChanged(int)), SLOT(refreshFontPreview()));
    connect(ui->consoleFont, SIGNAL(currentFontChanged(QFont)), SLOT(refreshFontPreview()));
    connect(ui->languageBox, SIGNAL(currentIndexChanged(int)), SLOT(languageIndexChanged(int)));
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

void MultiMCPage::on_instDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Instance Folder"), ui->instDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        if (FS::checkProblemticPathJava(QDir(cooked_dir)))
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
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Icons Folder"), ui->iconsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        ui->iconsDirTextBox->setText(cooked_dir);
    }
}
void MultiMCPage::on_modsDirBrowseBtn_clicked()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("Mods Folder"), ui->modsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists())
    {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        ui->modsDirTextBox->setText(cooked_dir);
    }
}

void MultiMCPage::languageIndexChanged(int index)
{
    auto languageCode = ui->languageBox->itemData(ui->languageBox->currentIndex()).toString();
    if(languageCode.isEmpty())
    {
        qWarning() << "Unknown language at index" << index;
        return;
    }
    auto translations = MMC->translations();
    translations->selectLanguage(languageCode);
    translations->updateLanguage(languageCode);
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
            qDebug() << "Selected index" << i << "channel id" << m_currentUpdateChannel;
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
    s->set("Language", ui->languageBox->itemData(ui->languageBox->currentIndex()).toString());

    if (ui->resetNotificationsBtn->isChecked())
    {
        s->set("ShownNotifications", QString());
    }

    // Updates
    s->set("AutoUpdate", ui->autoUpdateCheckBox->isChecked());
    s->set("UpdateChannel", m_currentUpdateChannel);
    auto original = s->get("IconTheme").toString();
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
    case 7:
        s->set("IconTheme", "flat");
        break;
    case 8:
        s->set("IconTheme", "custom");
        break;
    case 0:
    default:
        s->set("IconTheme", "multimc");
        break;
    }

    if(original != s->get("IconTheme"))
    {
        MMC->setIconTheme(s->get("IconTheme").toString());
    }

    auto originalAppTheme = s->get("ApplicationTheme").toString();
    auto newAppTheme = ui->themeComboBoxColors->currentData().toString();
    if(originalAppTheme != newAppTheme)
    {
        s->set("ApplicationTheme", newAppTheme);
        MMC->setApplicationTheme(newAppTheme, false);
    }

    // Console settings
    s->set("ShowConsole", ui->showConsoleCheck->isChecked());
    s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
    s->set("ShowConsoleOnError", ui->showConsoleErrorCheck->isChecked());
    QString consoleFontFamily = ui->consoleFont->currentFont().family();
    s->set("ConsoleFont", consoleFontFamily);
    s->set("ConsoleFontSize", ui->fontSizeBox->value());
    s->set("ConsoleMaxLines", ui->lineLimitSpinBox->value());
    s->set("ConsoleOverflowStop", ui->checkStopLogging->checkState() != Qt::Unchecked);

    // Folders
    // TODO: Offer to move instances to new instance folder.
    s->set("InstanceDir", ui->instDirTextBox->text());
    s->set("CentralModsDir", ui->modsDirTextBox->text());
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

    // Analytics
    if(!BuildConfig.ANALYTICS_ID.isEmpty())
    {
        s->set("Analytics", ui->analyticsCheck->isChecked());
    }
}
void MultiMCPage::loadSettings()
{
    auto s = MMC->settings();
    // Language
    {
        ui->languageBox->setModel(m_languageModel.get());
        ui->languageBox->setCurrentIndex(ui->languageBox->findData(s->get("Language").toString()));
    }

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
    else if (theme == "flat")
    {
        ui->themeComboBox->setCurrentIndex(7);
    }
    else if (theme == "custom")
    {
        ui->themeComboBox->setCurrentIndex(8);
    }
    else
    {
        ui->themeComboBox->setCurrentIndex(0);
    }

    {
        auto currentTheme = s->get("ApplicationTheme").toString();
        auto themes = MMC->getValidApplicationThemes();
        int idx = 0;
        for(auto &theme: themes)
        {
            ui->themeComboBoxColors->addItem(theme->name(), theme->id());
            if(currentTheme == theme->id())
            {
                ui->themeComboBoxColors->setCurrentIndex(idx);
            }
            idx++;
        }
    }

    // Console settings
    ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
    ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());
    ui->showConsoleErrorCheck->setChecked(s->get("ShowConsoleOnError").toBool());
    QString fontFamily = MMC->settings()->get("ConsoleFont").toString();
    QFont consoleFont(fontFamily);
    ui->consoleFont->setCurrentFont(consoleFont);

    bool conversionOk = true;
    int fontSize = MMC->settings()->get("ConsoleFontSize").toInt(&conversionOk);
    if(!conversionOk)
    {
        fontSize = 11;
    }
    ui->fontSizeBox->setValue(fontSize);
    refreshFontPreview();
    ui->lineLimitSpinBox->setValue(s->get("ConsoleMaxLines").toInt());
    ui->checkStopLogging->setChecked(s->get("ConsoleOverflowStop").toBool());

    // Folders
    ui->instDirTextBox->setText(s->get("InstanceDir").toString());
    ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
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

    // Analytics
    if(!BuildConfig.ANALYTICS_ID.isEmpty())
    {
        ui->analyticsCheck->setChecked(s->get("Analytics").toBool());
    }
}

void MultiMCPage::refreshFontPreview()
{
    int fontSize = ui->fontSizeBox->value();
    QString fontFamily = ui->consoleFont->currentFont().family();
    ui->fontPreview->clear();
    defaultFormat->setFont(QFont(fontFamily, fontSize));
    {
        QTextCharFormat format(*defaultFormat);
        format.setForeground(m_colors->getFront(MessageLevel::Error));
        // append a paragraph/line
        auto workCursor = ui->fontPreview->textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(tr("[Something/ERROR] A spooky error!"), format);
        workCursor.insertBlock();
    }
    {
        QTextCharFormat format(*defaultFormat);
        format.setForeground(m_colors->getFront(MessageLevel::Message));
        // append a paragraph/line
        auto workCursor = ui->fontPreview->textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(tr("[Test/INFO] A harmless message..."), format);
        workCursor.insertBlock();
    }
    {
        QTextCharFormat format(*defaultFormat);
        format.setForeground(m_colors->getFront(MessageLevel::Warning));
        // append a paragraph/line
        auto workCursor = ui->fontPreview->textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(tr("[Something/WARN] A not so spooky warning."), format);
        workCursor.insertBlock();
    }
}
