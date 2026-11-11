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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>

#include "Application.h"
#include "InstallLoaderDialog.h"
#include "meta/Index.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/widgets/PageContainer.h"
#include "ui/widgets/VersionSelectWidget.h"

namespace {
const QHash<QString, QString> g_loaderIcons = {
    { "net.fabricmc.fabric-loader", "fabricmc" }, { "net.minecraftforge", "forge" },
    { "com.mumfrey.liteloader", "liteloader" },   { "net.neoforged", "neoforged" },
    { "org.quiltmc.quilt-loader", "quiltmc" },
};

class InstallLoaderPage : public VersionSelectWidget, public BasePage {
    Q_OBJECT
   public:
    InstallLoaderPage(Meta::VersionList::Ptr list, const QString& currentVersion, const QString& gameVersionFilter)
        : VersionSelectWidget(nullptr), m_list(std::move(list))
    {
        if (gameVersionFilter.isEmpty()) {
            setEmptyString(tr("No versions are available"));
        } else {
            setEmptyString(tr("No versions are currently available for Minecraft %1").arg(gameVersionFilter));
            setExactIfPresentFilter(BaseVersionList::ParentVersionRole, gameVersionFilter);
        }

        if (!currentVersion.isEmpty()) {
            setCurrentVersion(currentVersion);
        }
    }

    QString id() const override { return m_list->uid(); }

    QString displayName() const override { return m_list->name(); }

    QIcon icon() const override { return QIcon::fromTheme(g_loaderIcons.value(m_list->uid(), "loadermods")); }

    void openedImpl() override
    {
        if (m_loaded) {
            return;
        }

        initialize(m_list.get());
        m_loaded = true;
    }

    void setParentContainer(BasePageContainer* container) override
    {
        auto* dialog = dynamic_cast<QDialog*>(dynamic_cast<PageContainer*>(container)->parent());
        connect(view(), &QAbstractItemView::doubleClicked, dialog, &QDialog::accept);
    }

    const Meta::VersionList::Ptr& list() const { return m_list; }

   private:
    const Meta::VersionList::Ptr m_list;
    bool m_loaded = false;
};

InstallLoaderPage* pageCast(BasePage* page)
{
    auto* result = dynamic_cast<InstallLoaderPage*>(page);
    Q_ASSERT(result != nullptr);
    return result;
}

void showNoLoadersError(QWidget* parent)
{
    const bool hasMetaOverride = !APPLICATION->settings()->get("MetaURLOverride").toString().isEmpty();

    QString message;
    if (!hasMetaOverride) {
        message = QObject::tr(
            "No installable mod loaders could be found. Local metadata is most likely outdated - make sure you are connected to the "
            "internet.");
    } else {
        message = QObject::tr(
            "No installable mod loaders could be found on the currently selected metadata server. You should ask the provider of the "
            "server to fix this. Until then, you may need to reset to the default metadata server to see loaders.");
    }

    auto* dialog = CustomMessageBox::selectable(parent, QObject::tr("No Loaders Found"), message, QMessageBox::Warning);

    auto* resetMetaCheckBox = new QCheckBox(QObject::tr("Reset metadata server"));
    if (hasMetaOverride) {
        dialog->setCheckBox(resetMetaCheckBox);
    }

    dialog->exec();

    if (resetMetaCheckBox->isChecked()) {
        APPLICATION->settings()->reset("MetaURLOverride");
    }
}

std::optional<InstallLoaderDialog::Result> chooseLoader(const QString& gameVersion, PackProfile* profile, QWidget* parent)
{
    auto* index = APPLICATION->metadataIndex();
    auto indexLoadTask = index->loadTask(Net::Mode::Online, false);

    QObject::connect(indexLoadTask.get(), &Task::failed, parent, [parent](const QString& reason) {
        auto* dialog = CustomMessageBox::selectable(parent, QObject::tr("Error Listing Loaders"),
                                                    QObject::tr("Failed to reload index: %1.").arg(reason), QMessageBox::Warning);
        dialog->open();
    });

    ProgressDialog progress(parent);
    progress.setSkipButton(true);
    if (progress.execWithTask(indexLoadTask.get()) != QDialog::Accepted) {
        return std::nullopt;
    }

    bool anyAvailable = std::ranges::any_of(index->lists(), [](const auto& list) { return list->installableLoader(); });
    if (!anyAvailable) {
        showNoLoadersError(parent);
        return std::nullopt;
    }

    InstallLoaderDialog dialog(gameVersion, profile, parent);
    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    return dialog.getResult();
}

bool resolveLoaderConflicts(const InstallLoaderDialog::Result& loader, PackProfile* profile, QWidget* parent)
{
    QList<ComponentPtr> conflicts;
    auto knownLoader = Component::KNOWN_MODLOADERS.find(loader.list->uid());
    if (knownLoader != Component::KNOWN_MODLOADERS.cend()) {
        for (const QString& conflictId : knownLoader->knownConflictingComponents) {
            const ComponentPtr conflictComponent = profile->getComponent(conflictId);
            if (conflictComponent && conflictComponent->isEnabled() && !conflictComponent->isCustom()) {
                conflicts.append(conflictComponent);
            }
        }
    }

    const QString targetVersion = QString("%1 %2").arg(loader.list->name(), loader.version->descriptor());

    for (const ComponentPtr& conflict : conflicts) {
        const QString conflictVersion = QString("%1 %2").arg(conflict->getName(), conflict->getVersion());
        auto* msgBox = CustomMessageBox::selectable(
            parent, QObject::tr("Installing a second loader"),
            QObject::tr("%1 is known to conflict with %2, which is enabled on this instance. Having both enabled at the same time will "
                        "likely break the instance.\n\nWhat would you like to do with %2?")
                .arg(targetVersion, conflictVersion),
            QMessageBox::Warning, QMessageBox::Cancel);
        QAbstractButton* keepButton = msgBox->addButton(QObject::tr("Keep it"), QMessageBox::AcceptRole);
        QAbstractButton* disableButton =
            conflict->canBeDisabled() ? msgBox->addButton(QObject::tr("Disable it"), QMessageBox::ActionRole) : nullptr;
        QAbstractButton* uninstallButton =
            conflict->isRemovable() ? msgBox->addButton(QObject::tr("Uninstall it"), QMessageBox::DestructiveRole) : nullptr;
        msgBox->exec();

        auto* clicked = msgBox->clickedButton();
        if (clicked == keepButton) {
            continue;
        }

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
}  // namespace

InstallLoaderDialog::InstallLoaderDialog(QString gameVersion, PackProfile* profile, QWidget* parent)
    : QDialog(parent)
    , m_gameVersion(std::move(gameVersion))
    , m_profile(profile)
    , m_container(new PageContainer(this, QString(), this))
    , m_buttons(new QDialogButtonBox(this))
{
    auto* layout = new QVBoxLayout(this);
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_container);

    auto* buttonLayout = new QHBoxLayout();
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    buttonLayout->setContentsMargins(0, 0, 6, 6);
#endif
    auto* refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::clicked, this, [this] { pageCast(m_container->selectedPage())->loadList(true); });
    buttonLayout->addWidget(refreshButton);

    m_buttons->setOrientation(Qt::Horizontal);
    m_buttons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(m_buttons);

    m_container->addButtons(buttonLayout);

    setWindowTitle(dialogTitle());
    setWindowModality(Qt::WindowModal);
    resize(520, 347);

    for (BasePage* page : m_container->getPages()) {
        connect(pageCast(page), &VersionSelectWidget::selectedVersionChanged, this, [this, page] {
            if (page->id() == m_container->selectedPage()->id()) {
                validate(m_container->selectedPage());
            }
        });
    }
    connect(m_container, &PageContainer::selectedPageChanged, this, [this](BasePage* previous, BasePage* current) { validate(current); });
    if (auto* selectedPage = m_container->selectedPage(); selectedPage != nullptr) {
        pageCast(selectedPage)->selectSearch();
        validate(selectedPage);
    }
}

std::optional<InstallLoaderDialog::Result> InstallLoaderDialog::choose(const QString& gameVersion, QWidget* parent)
{
    return chooseLoader(gameVersion, nullptr, parent);
}

bool InstallLoaderDialog::chooseAndInstall(PackProfile* profile, QWidget* parent)
{
    const auto loader = chooseLoader(profile->getComponentVersion("net.minecraft"), profile, parent);
    if (!loader.has_value()) {
        return false;
    }

    if (!resolveLoaderConflicts(loader.value(), profile, parent)) {
        return false;
    }

    profile->setComponentVersion(loader->list->uid(), loader->version->descriptor());
    if (const auto component = profile->getComponent(loader->list->uid()); !component->isEnabled()) {
        component->setEnabled(true);
    }

    profile->resolve(Net::Mode::Online);
    return true;
}

std::optional<InstallLoaderDialog::Result> InstallLoaderDialog::getResult() const
{
    if (m_container->selectedPage() == nullptr) {
        return std::nullopt;
    }

    auto* page = pageCast(m_container->selectedPage());
    const auto& selectedVersion = page->selectedVersion();
    if (selectedVersion == nullptr) {
        return std::nullopt;
    }

    return Result{
        .list = page->list(),
        .version = std::static_pointer_cast<Meta::Version>(selectedVersion),
    };
}

QList<BasePage*> InstallLoaderDialog::getPages()
{
    QList<BasePage*> result;
    for (const auto& list : APPLICATION->metadataIndex()->lists()) {
        if (!list->installableLoader()) {
            continue;
        }

        QString currentVersion;
        if (m_profile != nullptr) {
            currentVersion = m_profile->getComponentVersion(list->uid());
        }
        result.append(new InstallLoaderPage(list, currentVersion, m_gameVersion));
    }
    std::ranges::sort(result, [](BasePage* a, BasePage* b) { return a->displayName() < b->displayName(); });
    return result;
}

QString InstallLoaderDialog::dialogTitle()
{
    return tr("Install Loader");
}

void InstallLoaderDialog::validate(BasePage* page)
{
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(pageCast(page)->selectedVersion() != nullptr);
}

#include "InstallLoaderDialog.moc"
