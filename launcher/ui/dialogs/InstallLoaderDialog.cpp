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
    InstallLoaderPage(const QString& id, const QString& iconName, const QString& name, const Version& oldestVersion, PackProfile* profile)
        : VersionSelectWidget(nullptr), uid(id), iconName(iconName), name(name)
    {
        const QString minecraftVersion = profile->getComponentVersion("net.minecraft");
        setEmptyString(tr("No versions are currently available for Minecraft %1").arg(minecraftVersion));
        setExactIfPresentFilter(BaseVersionList::ParentVersionRole, minecraftVersion);

        if (oldestVersion != Version() && Version(minecraftVersion) < oldestVersion)
            setExactFilter(BaseVersionList::ParentVersionRole, "AAA");

        if (const QString currentVersion = profile->getComponentVersion(id); !currentVersion.isNull())
            setCurrentVersion(currentVersion);
    }

    QString id() const override { return uid; }
    QString displayName() const override { return name; }
    QIcon icon() const override { return QIcon::fromTheme(iconName); }

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
    const QString iconName;
    const QString name;
    bool loaded = false;
};

static InstallLoaderPage* pageCast(BasePage* page)
{
    auto result = dynamic_cast<InstallLoaderPage*>(page);
    Q_ASSERT(result != nullptr);
    return result;
}

InstallLoaderDialog::InstallLoaderDialog(PackProfile* profile, const QString& uid, QWidget* parent)
    : QDialog(parent), profile(profile), container(new PageContainer(this, QString(), this)), buttons(new QDialogButtonBox(this))
{
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
    connect(refreshButton, &QPushButton::clicked, this, [this] { pageCast(container->selectedPage())->loadList(true); });
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

    for (BasePage* page : container->getPages()) {
        if (page->id() == uid)
            container->selectPage(page->id());

        connect(pageCast(page), &VersionSelectWidget::selectedVersionChanged, this, [this, page] {
            if (page->id() == container->selectedPage()->id())
                validate(container->selectedPage());
        });
    }
    connect(container, &PageContainer::selectedPageChanged, this, [this](BasePage* previous, BasePage* current) { validate(current); });
    pageCast(container->selectedPage())->selectSearch();
    validate(container->selectedPage());
}

QList<BasePage*> InstallLoaderDialog::getPages()
{
    return { // NeoForge
             new InstallLoaderPage("net.neoforged", "neoforged", tr("NeoForge"), {}, profile),
             // Forge
             new InstallLoaderPage("net.minecraftforge", "forge", tr("Forge"), {}, profile),
             // Fabric
             new InstallLoaderPage("net.fabricmc.fabric-loader", "fabricmc", tr("Fabric"), Version("1.14"), profile),
             // Quilt
             new InstallLoaderPage("org.quiltmc.quilt-loader", "quiltmc", tr("Quilt"), Version("1.14"), profile),
             // LiteLoader
             new InstallLoaderPage("com.mumfrey.liteloader", "liteloader", tr("LiteLoader"), {}, profile)
    };
}

QString InstallLoaderDialog::dialogTitle()
{
    return tr("Install Loader");
}

void InstallLoaderDialog::validate(BasePage* page)
{
    buttons->button(QDialogButtonBox::Ok)->setEnabled(pageCast(page)->selectedVersion() != nullptr);
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
            tr("%1 is known to conflict with %2, which is already enabled on this instance. Having both enabled at the same time will "
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
    if (result == Accepted) {
        auto* page = pageCast(container->selectedPage());
        if (page->selectedVersion()) {
            if (!resolveLoaderConflicts(page))
                return;

            if (const ComponentPtr component = profile->getComponent(page->id()); component && !component->isEnabled())
                component->setEnabled(true);

            profile->setComponentVersion(page->id(), page->selectedVersion()->descriptor());
            profile->resolve(Net::Mode::Online);
        }
    }

    QDialog::done(result);
}
#include "InstallLoaderDialog.moc"
