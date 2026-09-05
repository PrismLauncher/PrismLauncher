// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MinecraftSettingsWidget.h"
#include "config/GlobalConfig.h"
#include "config/InstanceConfig.h"
#include "modplatform/ModIndex.h"
#include "ui_MinecraftSettingsWidget.h"

#include <QFileDialog>
#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"
#include "minecraft/PackProfile.h"
#include "minecraft/WorldList.h"
#include "minecraft/auth/AccountList.h"

MinecraftSettingsWidget::MinecraftSettingsWidget(MinecraftInstance* instance, QWidget* parent)
    : QWidget(parent), m_instance(std::move(instance)), m_ui(new Ui::MinecraftSettingsWidget)
{
    m_ui->setupUi(this);

    if (m_instance == nullptr) {
        m_ui->settingsTabs->removeTab(1);

        m_ui->openGlobalSettingsButton->setVisible(false);
        m_ui->instanceAccountGroupBox->hide();
        m_ui->serverJoinGroupBox->hide();
        m_ui->globalDataPacksGroupBox->hide();
        m_ui->loaderGroup->hide();
        m_ui->countGameTime->hide();

        m_ui->customCommands->setCheckable(false);
        m_ui->environmentVariables->setCheckable(false);
    } else {
        m_javaSettings = new JavaSettingsWidget(m_instance, this);
        m_ui->javaScrollArea->setWidget(m_javaSettings);

        m_ui->showGameTime->setText(tr("Show time &playing this instance"));
        m_ui->recordGameTime->setText(tr("&Record time playing this instance"));
        m_ui->showGlobalGameTime->hide();
        m_ui->showGameTimeWithoutDays->hide();

        m_ui->maximizedWarning->setText(
            tr("<span style=\" font-weight:600; color:#f5c211;\">Warning</span><span style=\" color:#f5c211;\">: The maximized option is "
               "not fully supported on this Minecraft version.</span>"));

        m_ui->consoleSettingsBox->setCheckable(true);
        m_ui->windowSizeGroupBox->setCheckable(true);
        m_ui->nativeWorkaroundsGroupBox->setCheckable(true);
        m_ui->perfomanceGroupBox->setCheckable(true);
        m_ui->gameTimeGroupBox->setCheckable(true);
        m_ui->legacySettingsGroupBox->setCheckable(true);

        m_quickPlaySingleplayer = m_instance->traits().contains("feature:is_quick_play_singleplayer");
        if (m_quickPlaySingleplayer) {
            auto worlds = m_instance->worldList();
            worlds->update();
            for (const auto& world : worlds->allWorlds()) {
                m_ui->worldsCb->addItem(world.folderName());
            }
        } else {
            m_ui->worldsCb->hide();
            m_ui->worldJoinButton->hide();
            m_ui->serverJoinAddressButton->setChecked(true);
            m_ui->serverJoinAddress->setEnabled(true);
            m_ui->serverJoinAddressButton->setStyleSheet("QRadioButton::indicator { width: 0px; height: 0px; }");
        }

        connect(m_ui->openGlobalSettingsButton, &QCommandLinkButton::clicked, this, &MinecraftSettingsWidget::openGlobalSettings);
        connect(m_ui->serverJoinAddressButton, &QAbstractButton::toggled, m_ui->serverJoinAddress, &QWidget::setEnabled);
        connect(m_ui->worldJoinButton, &QAbstractButton::toggled, m_ui->worldsCb, &QWidget::setEnabled);

        connect(m_ui->globalDataPacksGroupBox, &QGroupBox::toggled, this, [this](bool value) {
            if (value) {
                saveDataPacksPath();
            } else {
                m_instance->config().update().globalDataPacksPath = std::nullopt;
            }
        });
        connect(m_ui->dataPacksPathEdit, &QLineEdit::editingFinished, this, &MinecraftSettingsWidget::saveDataPacksPath);
        connect(m_ui->dataPacksPathBrowse, &QPushButton::clicked, this, &MinecraftSettingsWidget::selectDataPacksFolder);

        connect(m_ui->loaderGroup, &QGroupBox::toggled, this, [this](bool value) {
            if (value)
                saveSelectedLoaders();
            else
                m_instance->config().update().modDownloadLoaders = std::nullopt;
        });

        for (auto c : { m_ui->neoForge, m_ui->forge, m_ui->fabric, m_ui->quilt, m_ui->liteLoader, m_ui->babric, m_ui->btaBabric,
                        m_ui->legacyFabric, m_ui->ornithe, m_ui->rift }) {
            connect(c, &QCheckBox::stateChanged, this, &MinecraftSettingsWidget::saveSelectedLoaders);
        }
    }

    m_ui->maximizedWarning->hide();

    connect(m_ui->maximizedCheckBox, &QCheckBox::toggled, this,
            [this](const bool value) { m_ui->maximizedWarning->setVisible(value && (m_instance == nullptr || !m_instance->isLegacy())); });

#if !defined(Q_OS_LINUX)
    m_ui->perfomanceGroupBox->hide();
#endif

    if (!(APPLICATION->capabilities() & Application::SupportsGameMode)) {
        m_ui->enableFeralGamemodeCheck->setDisabled(true);
        m_ui->enableFeralGamemodeCheck->setToolTip(tr("Feral Interactive's GameMode could not be found on your system."));
    }

    if (!(APPLICATION->capabilities() & Application::SupportsMangoHud)) {
        m_ui->enableMangoHud->setEnabled(false);
        m_ui->enableMangoHud->setToolTip(tr("MangoHud could not be found on your system."));
    }

    connect(m_ui->useNativeOpenALCheck, &QAbstractButton::toggled, m_ui->lineEditOpenALPath, &QWidget::setEnabled);
    connect(m_ui->useNativeGLFWCheck, &QAbstractButton::toggled, m_ui->lineEditGLFWPath, &QWidget::setEnabled);
    connect(m_ui->useNativeSDLCheck, &QAbstractButton::toggled, m_ui->lineEditSDLPath, &QWidget::setEnabled);

    loadSettings();
}

MinecraftSettingsWidget::~MinecraftSettingsWidget()
{
    delete m_ui;
}

void MinecraftSettingsWidget::loadSettings()
{
    if (m_instance != nullptr) {
        const auto& conf = *m_instance->config();

        loadSettingsFrom(conf, *APPLICATION->config());

        m_ui->windowSizeGroupBox->setChecked(conf.gameWindow.has_value());
        m_ui->gameTimeGroupBox->setChecked(conf.gameTime.has_value());
        if (conf.gameTime.has_value()) {
            m_ui->countGameTime->setChecked(conf.countGameTime);
        }
        m_ui->consoleSettingsBox->setChecked(conf.console.has_value());
        m_ui->customCommands->setChecked(conf.commands.has_value());
        m_ui->environmentVariables->setChecked(conf.env.has_value());
        m_ui->legacySettingsGroupBox->setChecked(conf.legacy.has_value());
        m_ui->nativeWorkaroundsGroupBox->setChecked(conf.nativeLibraries.has_value());
        m_ui->perfomanceGroupBox->setChecked(conf.performance.has_value());

        // HACK: if we change enable state of child widgets while it's unchecked this creates inconsistency
        m_ui->serverJoinGroupBox->setChecked(true);

        if (const auto* server = std::get_if<InstanceConfig::ServerJoinTarget>(&conf.joinOnLaunch)) {
            m_ui->serverJoinAddress->setText(server->address);
            m_ui->serverJoinAddressButton->setChecked(true);
            m_ui->worldJoinButton->setChecked(false);
            m_ui->serverJoinAddress->setEnabled(true);
            m_ui->worldsCb->setEnabled(false);
        } else if (const auto* world = std::get_if<InstanceConfig::WorldJoinTarget>(&conf.joinOnLaunch)) {
            m_ui->worldsCb->setCurrentText(world->name);
            m_ui->serverJoinAddressButton->setChecked(false);
            m_ui->worldJoinButton->setChecked(true);
            m_ui->serverJoinAddress->setEnabled(false);
            m_ui->worldsCb->setEnabled(true);
        } else {
            m_ui->serverJoinGroupBox->setChecked(false);
            m_ui->serverJoinAddressButton->setChecked(true);
            m_ui->worldJoinButton->setChecked(false);
            m_ui->serverJoinAddress->setEnabled(true);
            m_ui->worldsCb->setEnabled(false);
        }

        m_ui->instanceAccountGroupBox->setChecked(conf.defaultAccount.has_value());
        updateAccountsMenu(conf);

        auto blockSignalsCheckBoxes = { m_ui->neoForge, m_ui->forge,     m_ui->fabric,       m_ui->quilt,   m_ui->liteLoader,
                                        m_ui->babric,   m_ui->btaBabric, m_ui->legacyFabric, m_ui->ornithe, m_ui->rift };
        m_ui->loaderGroup->blockSignals(true);
        for (auto* c : blockSignalsCheckBoxes) {
            c->blockSignals(true);
        }

        const auto& loaders = conf.modDownloadLoaders;
        m_ui->loaderGroup->setChecked(loaders.has_value());
        if (conf.modDownloadLoaders.has_value()) {
            m_ui->neoForge->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::NeoForge)));
            m_ui->forge->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Forge)));
            m_ui->fabric->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Fabric)));
            m_ui->quilt->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Quilt)));
            m_ui->liteLoader->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::LiteLoader)));
            m_ui->babric->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Babric)));
            m_ui->btaBabric->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::BTA)));
            m_ui->legacyFabric->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::LegacyFabric)));
            m_ui->ornithe->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Ornithe)));
            m_ui->rift->setChecked(loaders->contains(getModLoaderAsString(ModPlatform::Rift)));
        } else {
            auto instLoaders = m_instance->getPackProfile()->getSupportedModLoaders().value_or(ModPlatform::ModLoaderTypes(0));

            m_ui->neoForge->setChecked(instLoaders & ModPlatform::NeoForge);
            m_ui->forge->setChecked(instLoaders & ModPlatform::Forge);
            m_ui->fabric->setChecked(instLoaders & ModPlatform::Fabric);
            m_ui->quilt->setChecked(instLoaders & ModPlatform::Quilt);
            m_ui->liteLoader->setChecked(instLoaders & ModPlatform::LiteLoader);
            m_ui->babric->setChecked(instLoaders & ModPlatform::Babric);
            m_ui->btaBabric->setChecked(instLoaders & ModPlatform::BTA);
            m_ui->legacyFabric->setChecked(instLoaders & ModPlatform::LegacyFabric);
            m_ui->ornithe->setChecked(instLoaders & ModPlatform::Ornithe);
            m_ui->rift->setChecked(instLoaders & ModPlatform::Rift);
        }

        m_ui->loaderGroup->blockSignals(false);
        for (auto* c : blockSignalsCheckBoxes) {
            c->blockSignals(false);
        }

        m_ui->globalDataPacksGroupBox->blockSignals(true);
        m_ui->dataPacksPathEdit->blockSignals(true);
        m_ui->globalDataPacksGroupBox->setChecked(conf.globalDataPacksPath.has_value());
        if (conf.globalDataPacksPath.has_value()) {
            m_ui->dataPacksPathEdit->setText(*conf.globalDataPacksPath);
        }
        m_ui->globalDataPacksGroupBox->blockSignals(false);
        m_ui->dataPacksPathEdit->blockSignals(false);
    } else {
        const auto& conf = *APPLICATION->config();

        loadSettingsFrom(conf, conf);

        m_ui->showGlobalGameTime->setChecked(conf.showGlobalGameTime);
        m_ui->showGameTimeWithoutDays->setChecked(conf.showGameTimeWithoutDays);
    }
    if (m_javaSettings != nullptr)
        m_javaSettings->loadSettings();
}

template <typename T>
void MinecraftSettingsWidget::loadSettingsFrom(const T& conf, const GlobalConfig& global)
{
    // Game Window
    const auto gameWindow = conf.gameWindowOrGlobal(global);
    m_ui->maximizedCheckBox->setChecked(gameWindow.maximized);
    m_ui->windowWidthSpinBox->setValue(gameWindow.width);
    m_ui->windowHeightSpinBox->setValue(gameWindow.height);
    m_ui->closeAfterLaunchCheck->setChecked(gameWindow.hideLauncherOnOpen);
    m_ui->quitAfterGameStopCheck->setChecked(gameWindow.quitLauncherOnClose);

    // Game Time
    const auto gameTime = conf.gameTimeOrGlobal(global);
    m_ui->showGameTime->setChecked(gameTime.show);
    m_ui->recordGameTime->setChecked(gameTime.record);

    // Console
    const auto console = conf.consoleOrGlobal(global);
    m_ui->showConsoleCheck->setChecked(console.show);
    m_ui->autoCloseConsoleCheck->setChecked(console.autoClose);
    m_ui->showConsoleErrorCheck->setChecked(console.showOnError);

    // Custom commands
    const auto commands = conf.commandsOrGlobal(global);
    m_ui->customCommands->setCommands(commands.preLaunch, commands.wrapper, commands.postExit);

    // Environment variables
    m_ui->environmentVariables->setValue(conf.envOrGlobal(global));

    // Legacy Tweaks
    const auto legacy = conf.legacyOrGlobal(global);
    m_ui->onlineFixes->setChecked(legacy.onlineFixes);

    // Native Libraries
    const auto native = conf.nativeLibrariesOrGlobal(global);
    m_ui->useNativeGLFWCheck->setChecked(native.glfw);
    m_ui->lineEditGLFWPath->setText(native.customGLFWPath);
#ifdef Q_OS_LINUX
    m_ui->lineEditGLFWPath->setPlaceholderText(APPLICATION->m_detectedGLFWPath);
#else
    m_ui->lineEditGLFWPath->setPlaceholderText(tr("Path to %1 library file").arg(BuildConfig.GLFW_LIBRARY_NAME));
#endif
    m_ui->useNativeOpenALCheck->setChecked(native.openAL);
    m_ui->lineEditOpenALPath->setText(native.customOpenALPath);
#ifdef Q_OS_LINUX
    m_ui->lineEditOpenALPath->setPlaceholderText(APPLICATION->m_detectedOpenALPath);
#else
    m_ui->lineEditOpenALPath->setPlaceholderText(tr("Path to %1 library file").arg(BuildConfig.OPENAL_LIBRARY_NAME));
#endif
    m_ui->useNativeSDLCheck->setChecked(native.sdl);
    m_ui->lineEditSDLPath->setText(native.customSDLPath);
#ifdef Q_OS_LINUX
    m_ui->lineEditSDLPath->setPlaceholderText(APPLICATION->m_detectedSDLPath);
#else
    m_ui->lineEditSDLPath->setPlaceholderText(tr("Path to %1 library file").arg(BuildConfig.SDL_LIBRARY_NAME));
#endif

    // Performance
    const auto performance = conf.performanceOrGlobal(global);
    m_ui->enableFeralGamemodeCheck->setChecked(performance.enableFeralGamemode);
    m_ui->enableMangoHud->setChecked(performance.enableMangoHud);
    m_ui->useDiscreteGpuCheck->setChecked(performance.useDiscreteGpu);
    m_ui->useZink->setChecked(performance.useZink);
}

void MinecraftSettingsWidget::saveSettings()
{
    if (m_instance != nullptr) {
        auto& conf = m_instance->config().update();

        conf.console = std::nullopt;
        conf.gameWindow = std::nullopt;
        conf.commands = std::nullopt;
        conf.env = std::nullopt;
        conf.nativeLibraries = std::nullopt;
        conf.performance = std::nullopt;
        conf.gameTime = std::nullopt;
        conf.legacy = std::nullopt;

        saveSettingsTo(conf);

        if (m_ui->gameTimeGroupBox->isChecked()) {
            conf.countGameTime = m_ui->countGameTime->isChecked();
        } else {
            conf.countGameTime = true;
        }

        // Join server on launch
        if (m_ui->serverJoinGroupBox->isChecked()) {
            if (m_ui->serverJoinAddressButton->isChecked() || !m_quickPlaySingleplayer) {
                conf.joinOnLaunch = InstanceConfig::ServerJoinTarget{m_ui->serverJoinAddress->text()};
            } else {
                conf.joinOnLaunch = InstanceConfig::WorldJoinTarget{m_ui->serverJoinAddress->text()};
            }
        } else {
            conf.joinOnLaunch = std::monostate{};
        }

        // Use an account for this instance
        if (m_ui->instanceAccountGroupBox->isChecked()) {
            int accountIndex = m_ui->instanceAccountSelector->currentIndex();

            if (accountIndex != -1) {
                const MinecraftAccountPtr account = APPLICATION->accounts()->at(accountIndex);
                if (account != nullptr) {
                    conf.defaultAccount = account->profileId();
                }
            }
        } else {
            conf.defaultAccount = std::nullopt;
        }
    } else {
        auto& conf = APPLICATION->config().update();

        saveSettingsTo(conf);

        conf.showGlobalGameTime = m_ui->showGlobalGameTime->isChecked();
        conf.showGameTimeWithoutDays = m_ui->showGameTimeWithoutDays->isChecked();
    }

    if (m_javaSettings != nullptr)
        m_javaSettings->saveSettings();
}

template <typename T>
void MinecraftSettingsWidget::saveSettingsTo(T& conf) const
{
    // Console
    if (!m_ui->consoleSettingsBox->isCheckable() || m_ui->consoleSettingsBox->isChecked()) {
        conf.console = GlobalConfig::ConsoleOverrides{
            .show = m_ui->showConsoleCheck->isChecked(),
            .autoClose = m_ui->autoCloseConsoleCheck->isChecked(),
            .showOnError = m_ui->showConsoleErrorCheck->isChecked(),
        };
    }

    // Game Window

    if (!m_ui->windowSizeGroupBox->isCheckable() || m_ui->windowSizeGroupBox->isChecked()) {
        conf.gameWindow = GlobalConfig::GameWindowOverrides{
            .maximized = m_ui->maximizedCheckBox->isChecked(),
            .width = m_ui->windowWidthSpinBox->value(),
            .height = m_ui->windowHeightSpinBox->value(),
            .hideLauncherOnOpen = m_ui->closeAfterLaunchCheck->isChecked(),
            .quitLauncherOnClose = m_ui->quitAfterGameStopCheck->isChecked(),
        };
    }

    // Custom Commands
    if (m_ui->customCommands->checked()) {
        conf.commands = GlobalConfig::CommandOverrides{
            .preLaunch = m_ui->customCommands->prelaunchCommand(),
            .wrapper = m_ui->customCommands->wrapperCommand(),
            .postExit = m_ui->customCommands->postexitCommand(),
        };
    }

    // Environment Variables
    if (m_ui->environmentVariables->checked()) {
        conf.env = m_ui->environmentVariables->value();
    }

    // Workarounds
    if (m_ui->nativeWorkaroundsGroupBox->isChecked()) {
        conf.nativeLibraries = GlobalConfig::NativeLibraryOverrides{
            .glfw = m_ui->useNativeGLFWCheck->isChecked(),
            .customGLFWPath = m_ui->lineEditGLFWPath->text(),
            .openAL = m_ui->useNativeOpenALCheck->isChecked(),
            .customOpenALPath = m_ui->lineEditOpenALPath->text(),
            .sdl = m_ui->useNativeSDLCheck->isChecked(),
            .customSDLPath = m_ui->lineEditSDLPath->text(),
        };
    }

    // Performance
    if (m_ui->perfomanceGroupBox->isChecked()) {
        conf.performance = GlobalConfig::PerformanceOverrides{
            .enableFeralGamemode = m_ui->enableFeralGamemodeCheck->isChecked(),
            .enableMangoHud = m_ui->enableMangoHud->isChecked(),
            .useDiscreteGpu = m_ui->useDiscreteGpuCheck->isChecked(),
            .useZink = m_ui->useZink->isChecked(),
        };
    }

    // Game time
    if (!m_ui->gameTimeGroupBox->isCheckable() || m_ui->gameTimeGroupBox->isChecked()) {
        conf.gameTime = GlobalConfig::GameTimeOverrides{
            .show = m_ui->showGameTime->isChecked(),
            .record = m_ui->recordGameTime->isChecked(),
        };
    }

    if (m_ui->legacySettingsGroupBox->isChecked()) {
        conf.legacy = GlobalConfig::LegacyOverrides{
            .onlineFixes = m_ui->onlineFixes->isChecked(),
        };
    }
}

void MinecraftSettingsWidget::openGlobalSettings()
{
    const QString id = m_ui->settingsTabs->currentWidget()->objectName();

    qDebug() << id;

    if (id == "javaPage")
        APPLICATION->ShowGlobalSettings(this, "java-settings");
    else  // TODO select tab
        APPLICATION->ShowGlobalSettings(this, "minecraft-settings");
}

void MinecraftSettingsWidget::updateAccountsMenu(const InstanceConfig& conf)
{
    m_ui->instanceAccountSelector->clear();
    auto accounts = APPLICATION->accounts();
    int accountIndex = -1;
    if (conf.defaultAccount.has_value()) {
        accountIndex = accounts->findAccountByProfileId(*conf.defaultAccount);
    }

    for (int i = 0; i < accounts->count(); i++) {
        MinecraftAccountPtr account = accounts->at(i);

        QIcon face = account->getFace();

        if (face.isNull())
            face = QIcon::fromTheme("noaccount");

        m_ui->instanceAccountSelector->addItem(face, account->profileName(), i);
        if (i == accountIndex)
            m_ui->instanceAccountSelector->setCurrentIndex(i);
    }
}

bool MinecraftSettingsWidget::isQuickPlaySupported()
{
    return m_instance->traits().contains("feature:is_quick_play_singleplayer");
}

void MinecraftSettingsWidget::saveSelectedLoaders()
{
    QStringList loaders;

    if (m_ui->neoForge->isChecked())
        loaders << getModLoaderAsString(ModPlatform::NeoForge);
    if (m_ui->forge->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Forge);
    if (m_ui->fabric->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Fabric);
    if (m_ui->quilt->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Quilt);
    if (m_ui->liteLoader->isChecked())
        loaders << getModLoaderAsString(ModPlatform::LiteLoader);
    if (m_ui->babric->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Babric);
    if (m_ui->btaBabric->isChecked())
        loaders << getModLoaderAsString(ModPlatform::BTA);
    if (m_ui->legacyFabric->isChecked())
        loaders << getModLoaderAsString(ModPlatform::LegacyFabric);
    if (m_ui->ornithe->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Ornithe);
    if (m_ui->rift->isChecked())
        loaders << getModLoaderAsString(ModPlatform::Rift);

    m_instance->config().update().modDownloadLoaders = loaders;
}

void MinecraftSettingsWidget::saveDataPacksPath()
{
    if (QDir::separator() != '/')
        m_ui->dataPacksPathEdit->setText(m_ui->dataPacksPathEdit->text().replace(QDir::separator(), '/'));

    m_instance->config().update().globalDataPacksPath = m_ui->dataPacksPathEdit->text();
}

void MinecraftSettingsWidget::selectDataPacksFolder()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Global Data Packs Folder"), m_instance->gameRoot());

    if (path.isEmpty())
        return;

    // if it's inside the instance dir, set path relative to .minecraft
    // (so that if it's directly in instance dir it will still lead with .. but more than two levels up are kept absolute)

    const QUrl instanceRootUrl = QUrl::fromLocalFile(m_instance->instanceRoot());
    const QUrl pathUrl = QUrl::fromLocalFile(path);

    if (instanceRootUrl.isParentOf(pathUrl))
        path = QDir(m_instance->gameRoot()).relativeFilePath(path);

    m_ui->dataPacksPathEdit->setText(path);
    m_instance->config().update().globalDataPacksPath = path;
}
