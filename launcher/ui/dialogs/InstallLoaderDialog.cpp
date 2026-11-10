// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 */

#include "InstallLoaderDialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "meta/Index.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/widgets/PageContainer.h"
#include "ui/widgets/VersionSelectWidget.h"

class InstallLoaderPage : public VersionSelectWidget, public BasePage {
    Q_OBJECT
   public:
    static PageInfo page(const QString& uid, const QString& iconName, const char* name, const Version& oldestVersion, PackProfile* profile)
    {
        return {
            .id = uid,
            .displayName = name,
            .icon = QIcon::fromTheme(iconName),
            .constructor = [uid, oldestVersion = oldestVersion, profile,
                            name](QWidget* parent) { return new InstallLoaderPage(uid, name, oldestVersion, profile, parent); },
        };
    }

    InstallLoaderPage(const QString& uid,
                      const QString& name,
                      const Version& oldestVersion,
                      PackProfile* profile,
                      QWidget* parent = nullptr)
        : VersionSelectWidget(parent), uid(uid), name(name)
    {
        const QString minecraftVersion = profile->getComponentVersion("net.minecraft");
        setEmptyString(tr("No versions are currently available for Minecraft %1").arg(minecraftVersion));
        setExactIfPresentFilter(BaseVersionList::ParentVersionRole, minecraftVersion);

        if (oldestVersion != Version() && Version(minecraftVersion) < oldestVersion)
            setExactFilter(BaseVersionList::ParentVersionRole, "AAA");

        if (const QString currentVersion = profile->getComponentVersion(uid); !currentVersion.isNull())
            setCurrentVersion(currentVersion);
    }

    QString id() const { return uid; }

    QString displayName() const { return name; }

    void openedImpl() override
    {
        if (loaded)
            return;

        const auto versions = APPLICATION->metadataIndex()->get(uid);
        if (!versions)
            return;

        initialize(versions.get());
        loaded = true;
    }

    void setParentContainer(BasePageContainer* container) override
    {
        auto dialog = dynamic_cast<QDialog*>(dynamic_cast<PageContainer*>(container)->parent());
        connect(view(), &QAbstractItemView::doubleClicked, dialog, &QDialog::accept);
    }

   private:
    const QString uid;
    const QString name;
    bool loaded = false;
};

static InstallLoaderPage* pageCast(QWidget* page)
{
    auto result = dynamic_cast<InstallLoaderPage*>(page);
    Q_ASSERT(result != nullptr);
    return result;
}

InstallLoaderDialog::InstallLoaderDialog(PackProfile* profile, const QString& uid, QWidget* parent)
    : QDialog(parent), profile(profile), container(new PageContainer(this, QString(), this)), buttons(new QDialogButtonBox(this))
{
    // FIXME: we need this since the code relies a little on iterating the pages
    container->forceLoadWidgets();

    auto layout = new QVBoxLayout(this);
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(container);

    auto buttonLayout = new QHBoxLayout(this);
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    buttonLayout->setContentsMargins(0, 0, 6, 6);
#endif
    auto refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        if (auto* page = dynamic_cast<InstallLoaderPage*>(container->selectedPageWidget())) {
            page->loadList(true);
        }
    });
    buttonLayout->addWidget(refreshButton);

    buttons->setOrientation(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(buttons);

    container->addButtons(buttonLayout);

    setWindowTitle(dialogTitle());
    setWindowModality(Qt::WindowModal);
    resize(520, 347);

    for (QWidget* page : container->pageWidgets()) {
        connect(pageCast(page), &VersionSelectWidget::selectedVersionChanged, this, [this, page] {
            if (page == container->selectedPageWidget()) {
                validate(container->selectedPageWidget());
            }
        });
    }
    container->selectPage(uid);

    connect(container, &PageContainer::selectedPageChanged, this, [this](QWidget* /*previous*/, QWidget* current) { validate(current); });
    if (container->selectedPageWidget() != nullptr) {
        pageCast(container->selectedPageWidget())->selectSearch();
    }
    validate(container->selectedPageWidget());
}

QList<PageInfo> InstallLoaderDialog::getPages()
{
    return { // NeoForge
             InstallLoaderPage::page("net.neoforged", "neoforged", "NeoForge", {}, profile),
             // Forge
             InstallLoaderPage::page("net.minecraftforge", "forge", "Forge", {}, profile),
             // Fabric
             InstallLoaderPage::page("net.fabricmc.fabric-loader", "fabricmc", "Fabric", Version("1.14"), profile),
             // Quilt
             InstallLoaderPage::page("org.quiltmc.quilt-loader", "quiltmc", "Quilt", Version("1.14"), profile),
             // LiteLoader
             InstallLoaderPage::page("com.mumfrey.liteloader", "liteloader", "LiteLoader", {}, profile)
    };
}

QString InstallLoaderDialog::dialogTitle()
{
    return tr("Install Loader");
}

void InstallLoaderDialog::validate(QWidget* page)
{
    buttons->button(QDialogButtonBox::Ok)->setEnabled(page != nullptr && pageCast(page)->selectedVersion() != nullptr);
}

bool InstallLoaderDialog::resolveLoaderConflicts(InstallLoaderPage* page)
{
    QList<ComponentPtr> conflicts;
    auto knownLoader = Component::KNOWN_MODLOADERS.find(page->id());
    if (knownLoader != Component::KNOWN_MODLOADERS.cend()) {
        for (const QString& conflictId : knownLoader->knownConflictingComponents) {
            const ComponentPtr conflictComponent = profile->getComponent(conflictId);
            if (conflictComponent && conflictComponent->isEnabled() && !conflictComponent->isCustom())
                conflicts.append(conflictComponent);
        }
    }

    const QString targetVersion = QString("%1 %2").arg(page->displayName(), page->selectedVersion()->descriptor());

    for (const ComponentPtr& conflict : conflicts) {
        const QString conflictVersion = QString("%1 %2").arg(conflict->getName(), conflict->getVersion());
        auto* msgBox = CustomMessageBox::selectable(
            this, tr("Installing a second loader"),
            tr("%1 is known to conflict with %2, which is enabled on this instance. Having both enabled at the same time will "
               "likely break the instance.\n\nWhat would you like to do with %2?")
                .arg(targetVersion, conflictVersion),
            QMessageBox::Warning, QMessageBox::Cancel);
        QAbstractButton* keepButton = msgBox->addButton(tr("Keep it"), QMessageBox::AcceptRole);
        QAbstractButton* disableButton = conflict->canBeDisabled() ? msgBox->addButton(tr("Disable it"), QMessageBox::ActionRole) : nullptr;
        QAbstractButton* uninstallButton =
            conflict->isRemovable() ? msgBox->addButton(tr("Uninstall it"), QMessageBox::DestructiveRole) : nullptr;
        msgBox->exec();

        auto* clicked = msgBox->clickedButton();
        if (clicked == keepButton)
            continue;
        if (disableButton && clicked == disableButton) {
            conflict->setEnabled(false);
            profile->resolve(Net::Mode::Online);
            continue;
        }
        if (uninstallButton && clicked == uninstallButton) {
            profile->remove(conflict->getID());
            profile->resolve(Net::Mode::Online);
            continue;
        }
        return false;
    }
    return true;
}

void InstallLoaderDialog::done(int result)
{
    if (result == Accepted && container->selectedPageWidget() != nullptr) {
        auto* page = pageCast(container->selectedPageWidget());
        if (page->selectedVersion()) {
            if (!resolveLoaderConflicts(page))
                return;

            if (const ComponentPtr component = profile->getComponent(page->id()); component && !component->isEnabled())
                component->setEnabled(true);

            profile->setComponentVersion(page->id(), page->selectedVersion()->descriptor());
            if (auto component = profile->getComponent(page->id())) {
                component->setEnabled(true);
            }
            profile->resolve(Net::Mode::Online);
        }
    }

    QDialog::done(result);
}
#include "InstallLoaderDialog.moc"
