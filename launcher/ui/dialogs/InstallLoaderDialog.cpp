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
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "meta/Index.h"
#include "minecraft/BabricCreationTask.h"
#include "minecraft/PackProfile.h"
#include "ui/widgets/PageContainer.h"
#include "ui/widgets/VersionSelectWidget.h"

class InstallLoaderPage : public VersionSelectWidget, public BasePage {
    Q_OBJECT
   public:
    InstallLoaderPage(const QString& id,
                      const QString& iconName,
                      const QString& name,
                      const Version& oldestVersion,
                      PackProfile* profile)
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

class InstallBabricPage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    explicit InstallBabricPage(PackProfile* profile, QWidget* parent = nullptr)
        : QWidget(parent), m_profile(profile)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setOpenExternalLinks(true);
        m_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(m_label);
        layout->addStretch();

        updateLabel();
    }

    QString id() const override { return "babric"; }
    QString displayName() const override { return tr("Babric"); }
    QIcon icon() const override { return QIcon::fromTheme("fabricmc"); }

    bool isCompatible() const
    {
        const QString mc = m_profile->getComponentVersion("net.minecraft");
        return mc.isEmpty() || mc == "b1.7.3";
    }

    bool install()
    {
        if (!BabricCreationTask::installPatches(m_profile))
            return false;
        m_profile->resolve(Net::Mode::Online);
        return true;
    }

   private:
    void updateLabel()
    {
        if (isCompatible()) {
            m_label->setText(
                tr("<b>Babric</b> is a Fabric-based mod loader "
                   "for <b>Minecraft Beta 1.7.3</b>."
                   "<br><br>"
                   "Clicking <b>OK</b> will:"
                   "<ul>"
                   "<li>Replace this instance's Minecraft version with <b>b1.7.3</b></li>"
                   "<li>Install <b>Fabric Loader 0.18.4</b> and the Babric mapping layer</li>"
                   "<li>Configure the mod browser to show Babric-compatible mods</li>"
                   "</ul>"));
        } else {
            const QString mc = m_profile->getComponentVersion("net.minecraft");
            m_label->setText(
                tr("No versions are currently available for Minecraft %1."
                   "<br><br>"
                   "Babric only supports <b>Minecraft Beta 1.7.3</b>.").arg(mc));
        }
    }

    PackProfile* m_profile = nullptr;
    QLabel* m_label = nullptr;
};


static InstallLoaderPage* asLoaderPage(BasePage* page)
{
    return dynamic_cast<InstallLoaderPage*>(page);
}

static InstallBabricPage* asBabricPage(BasePage* page)
{
    return dynamic_cast<InstallBabricPage*>(page);
}

InstallLoaderDialog::InstallLoaderDialog(PackProfile* profile, const QString& uid, QWidget* parent)
    : QDialog(parent)
    , profile(profile)
    , container(new PageContainer(this, QString(), this))
    , buttons(new QDialogButtonBox(this))
{
    auto* layout = new QVBoxLayout(this);
    // small margins look ugly on macOS on modal windows
    #ifndef Q_OS_MACOS
    layout->setContentsMargins(0, 0, 0, 0);
    #endif
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(container);

    auto* buttonLayout = new QHBoxLayout();
    // small margins look ugly on macOS on modal windows
    #ifndef Q_OS_MACOS
    buttonLayout->setContentsMargins(0, 0, 6, 6);
    #endif

    auto* refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        auto* page = container->selectedPage();
        if (auto* lp = asLoaderPage(page))
            lp->loadList(true);
        // Babric has no remote list to refresh.
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

    // Pre-select requested uid (if any) and wire version-selection → OK state.
    for (BasePage* page : container->getPages()) {
        if (!uid.isEmpty() && page->id() == uid)
            container->selectPage(page->id());

        if (auto* lp = dynamic_cast<InstallLoaderPage*>(page)) {
            connect(lp, &VersionSelectWidget::selectedVersionChanged, this, [this, page] {
                if (page == container->selectedPage())
                    validate(container->selectedPage());
            });
        }
    }

    connect(container, &PageContainer::selectedPageChanged, this,
            [this, refreshButton](BasePage* /*prev*/, BasePage* current) {
                refreshButton->setVisible(!asBabricPage(current));
                validate(current);
            });

    if (auto* lp = asLoaderPage(container->selectedPage()))
        lp->selectSearch();

    refreshButton->setVisible(!asBabricPage(container->selectedPage()));
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
			 // Babric
			 new InstallBabricPage(profile),
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
    bool ok = false;
    if (auto* bp = asBabricPage(page)) {
        ok = bp->isCompatible();
    } else if (auto* lp = asLoaderPage(page)) {
        ok = lp->selectedVersion() != nullptr;
    }
    buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void InstallLoaderDialog::done(int result)
{
    if (result == Accepted) {
        BasePage* page = container->selectedPage();

        if (auto* bp = asBabricPage(page)) {
            if (!bp->install()) {
                QMessageBox::critical(
                    this, tr("Install Error"),
                    tr("Failed to install Babric.\n\n"
                       "Run the launcher from a terminal to see details:\n"
                       "  cd install && .\\prismlauncher.exe 2>&1"));
                return;  // keep dialog open so user can try again or cancel
            }
        } else if (auto* lp = asLoaderPage(page)) {
            if (lp->selectedVersion()) {
                profile->setComponentVersion(lp->id(), lp->selectedVersion()->descriptor());
                profile->resolve(Net::Mode::Online);
            }
        }
    }

    QDialog::done(result);
}
#include "InstallLoaderDialog.moc"
