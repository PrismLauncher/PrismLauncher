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

class InstallLoaderPage : public VersionSelectWidget, public BasePage {
   public:
    InstallLoaderPage(const QString& id,
               const QString& icon,
               const QString& name,
               // "lightweight" loaders are independent to any game version
               const bool lightweight,
               const std::shared_ptr<PackProfile> profile,
               QWidget* parent = nullptr)
        : VersionSelectWidget(parent), m_id(id), m_icon(icon), m_name(name)
    {
        const QString minecraftVersion = profile->getComponentVersion("net.minecraft");
        setEmptyString(tr("No versions are currently available for Minecraft %1").arg(minecraftVersion));
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

InstallLoaderPage* pageCast(BasePage* page)
{
    auto result = dynamic_cast<InstallLoaderPage*>(page);
    Q_ASSERT(result != nullptr);
    return result;
}

InstallLoaderDialog::InstallLoaderDialog(std::shared_ptr<PackProfile> profile, const QString& uid, QWidget* parent)
    : QDialog(parent), m_profile(profile), m_container(new PageContainer(this)), m_buttons(new QDialogButtonBox(this))
{
    auto layout = new QVBoxLayout(this);

    m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_container);

    auto buttonLayout = new QHBoxLayout(this);

    auto refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::pressed, this, [this] {
        pageCast(m_container->selectedPage())->loadList();
    });
    buttonLayout->addWidget(refreshButton);

    m_buttons->setOrientation(Qt::Horizontal);
    m_buttons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(m_buttons);

    layout->addLayout(buttonLayout);

    setWindowTitle(dialogTitle());
    resize(650, 400);

    pageCast(m_container->selectedPage())->selectSearch();
    for (BasePage* page : m_container->getPages())
        if (page->id() == uid)
            m_container->selectPage(page->id());
}

QList<BasePage*> InstallLoaderDialog::getPages()
{
    return { // Forge
             new InstallLoaderPage("net.minecraftforge", "forge", tr("Forge"), false, m_profile, this),
             // Fabric
             new InstallLoaderPage("net.fabricmc.fabric-loader", "fabricmc-small", tr("Fabric"), true, m_profile, this),
             // Quilt
             new InstallLoaderPage("org.quiltmc.quilt-loader", "quiltmc", tr("Quilt"), true, m_profile, this),
             // LiteLoader
             new InstallLoaderPage("com.mumfrey.liteloader", "liteloader", tr("LiteLoader"), false, m_profile, this)
    };
}

QString InstallLoaderDialog::dialogTitle()
{
    return tr("Install Loader");
}

void InstallLoaderDialog::done(int result)
{
    if (result == Accepted) {
        auto* page = pageCast(m_container->selectedPage());
        if (page->selectedVersion()) {
            m_profile->setComponentVersion(page->id(), page->selectedVersion()->descriptor());
            m_profile->resolve(Net::Mode::Online);
        }
    }

    QDialog::done(result);
}
