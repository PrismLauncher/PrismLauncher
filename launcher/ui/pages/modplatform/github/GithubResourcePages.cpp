// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
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

#include "GithubResourcePages.h"

#include <algorithm>
#include <utility>

#include "minecraft/MinecraftInstance.h"

#include "modplatform/github/GithubAPI.h"

#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/widgets/ModFilterWidget.h"

namespace ResourceDownload {

namespace {

/** A mod model for GitHub, where releases carry no loader / Minecraft-version metadata. */
class GithubModModel : public ModModel {
   public:
    using ModModel::ModModel;

   protected:
    bool checkVersionFilters(const ModPlatform::IndexedVersion& v) override
    {
        if (!m_filter) {
            return true;
        }

        // GitHub releases are not annotated with loaders, sides or Minecraft versions, so
        // filtering on those would hide every version. Only the release-type filter applies.
        return !optedOut(v) && (m_filter->releases.empty() ||
                                std::find(m_filter->releases.cbegin(), m_filter->releases.cend(), v.versionType) !=
                                    m_filter->releases.cend());
    }
};

/** A mod page for GitHub that swaps in the GitHub-tolerant model. */
class GithubModPage : public ModPage {
   public:
    GithubModPage(ResourceDownloadDialog* dialog, BaseInstance& instance, ResourceProviderData provider)
        : ModPage(dialog, instance, std::move(provider), &GithubAPI::get(),
                  ModFilterWidget::create(&static_cast<MinecraftInstance&>(instance), true))
    {
        auto* oldModel = m_model;
        auto* model = new GithubModModel(instance, getDialog()->getBaseModel(), &GithubAPI::get(), debugName(), metaEntryBase());
        model->setFilter(getFilter());
        m_model = model;
        m_ui->packView->setModel(m_model);
        oldModel->disconnect();
        oldModel->deleteLater();

        connect(m_model, &ResourceModel::versionListUpdated, this, &ResourcePage::versionListUpdated);
        connect(m_model, &ResourceModel::projectInfoUpdated, this, &ResourcePage::updateUi);
        connect(m_model, &QAbstractListModel::modelReset, this, &ResourcePage::modelReset);
    }
};

ResourceProviderData prepareGithub()
{
    return {
        .displayName = Github::displayName(),
        .icon = Github::icon(),
        .id = Github::id(),
        .metaEntryBase = Github::metaEntryBase(),
        .debugName = Github::debugName(),
    };
}

}  // namespace

ModPage* Github::createModPage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new GithubModPage(dialog, instance, prepareGithub());
}

}  // namespace ResourceDownload
