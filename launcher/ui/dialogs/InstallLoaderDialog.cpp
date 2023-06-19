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
#include "ui/widgets/PageContainer.h"
#include "ui/widgets/VersionSelectWidget.h"

class LoaderPage : public VersionSelectWidget, public BasePage {
   public:
    LoaderPage(const QString&& id,
               const QString&& icon,
               const QString&& name,
               // "lightweight" loaders are independent to any game version
               const bool lightweight,
               const std::shared_ptr<PackProfile> profile,
               QWidget* parent = nullptr)
        : VersionSelectWidget(parent), m_id(std::move(id)), m_icon(std::move(icon)), m_name(std::move(name))
    {
        const QString minecraftVersion = profile->getComponentVersion("net.minecraft");
        setEmptyErrorString(tr("No versions are currently available for Minecraft %1").arg(minecraftVersion));
        if (!lightweight)
            setExactFilter(BaseVersionList::ParentVersionRole, minecraftVersion);

        if (const QString currentVersion = profile->getComponentVersion(id); !currentVersion.isNull())
            setCurrentVersion(currentVersion);
    }

    QString id() const override { return m_id; }
    QString displayName() const override { return m_name; }
    QIcon icon() const override { return APPLICATION->getThemedIcon(m_icon); }

    void openedImpl() override
    {
        if (m_loaded)
            return;

        const auto versions = APPLICATION->metadataIndex()->get(m_id);
        if (!versions)
            return;

        initialize(versions.get());
        m_loaded = true;
    }

   private:
    const QString m_id;
    const QString m_icon;
    const QString m_name;
    bool m_loaded = false;
};

InstallLoaderDialog::InstallLoaderDialog(std::shared_ptr<PackProfile> profile, QWidget* parent)
    : QDialog(parent), m_profile(profile), m_container(new PageContainer(this))
{
    auto layout = new QVBoxLayout(this);

    m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_container);

    auto buttonLayout = new QHBoxLayout(this);

    auto refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::pressed, this, [this] {
        LoaderPage* page = dynamic_cast<LoaderPage*>(m_container->selectedPage());
        Q_ASSERT(page != nullptr);
        page->loadList();
    });
    buttonLayout->addWidget(refreshButton);

    auto buttons = new QDialogButtonBox(this);
    buttons->setOrientation(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(buttons);

    layout->addLayout(buttonLayout);

    setWindowTitle(dialogTitle());
    resize(650, 400);
}

QList<BasePage*> InstallLoaderDialog::getPages()
{
    return { // Forge
             new LoaderPage("net.minecraftforge", "forge-loader", tr("Forge"), false, m_profile, this),
             // Fabric
             new LoaderPage("net.fabricmc.fabric-loader", "fabric-loader", tr("Fabric"), true, m_profile, this),
             // Quilt
             new LoaderPage("org.quiltmc.quilt-loader", "quilt-loader", tr("Quilt"), true, m_profile, this),
             // LiteLoader
             new LoaderPage("com.mumfrey.liteloader", "liteloader", tr("LiteLoader"), false, m_profile, this)
    };
}

QString InstallLoaderDialog::dialogTitle()
{
    return tr("Install Loader");
}

void InstallLoaderDialog::done(int result)
{
    if (result == Accepted) {
        LoaderPage* page = dynamic_cast<LoaderPage*>(m_container->selectedPage());
        Q_ASSERT(page != nullptr);

        if (page->selectedVersion()) {
            m_profile->setComponentVersion(page->id(), page->selectedVersion()->descriptor());
            m_profile->resolve(Net::Mode::Online);
        }
    }

    QDialog::done(result);
}
