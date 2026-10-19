// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginsPage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "DesktopServices.h"
#include "plugins/Plugin.h"
#include "plugins/PluginManager.h"

namespace {

enum Column { NameColumn, IdColumn, VersionColumn, StatusColumn };

}  // namespace

PluginsPage::PluginsPage(QWidget* parent) : QWidget(parent), m_manager(APPLICATION->plugins())
{
    setObjectName(QStringLiteral("pluginsPage"));

    m_intro = new QLabel(this);
    m_intro->setWordWrap(true);

    m_list = new QTreeWidget(this);
    m_list->setRootIsDecorated(false);
    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->header()->setStretchLastSection(true);

    m_details = new QLabel(this);
    m_details->setWordWrap(true);
    m_details->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_details->setTextFormat(Qt::PlainText);

    m_openFolder = new QPushButton(this);
    m_reload = new QPushButton(this);
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_openFolder);
    buttons->addWidget(m_reload);
    buttons->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_intro);
    layout->addWidget(m_list, 1);
    layout->addWidget(m_details);
    layout->addLayout(buttons);

    connect(m_list, &QTreeWidget::itemChanged, this, &PluginsPage::onItemChanged);
    connect(m_list, &QTreeWidget::currentItemChanged, this, &PluginsPage::updateDetails);
    connect(m_openFolder, &QPushButton::clicked, this, [this] { DesktopServices::openPath(m_manager->pluginsDir(), true); });
    connect(m_reload, &QPushButton::clicked, m_manager, &PluginManager::discover);
    connect(m_manager, &PluginManager::pluginsChanged, this, &PluginsPage::rebuild);
    connect(m_manager, &PluginManager::pluginChanged, this, &PluginsPage::onPluginChanged);

    retranslate();
    rebuild();
}

void PluginsPage::retranslate()
{
    m_intro->setText(
        tr("Plugins are Lua scripts that extend the launcher. They are loaded from the plugins folder and start disabled; "
           "only enable plugins you trust."));
    m_list->setHeaderLabels({ tr("Name"), tr("ID"), tr("Version"), tr("Status") });
    m_openFolder->setText(tr("Open Folder"));
    m_reload->setText(tr("Reload"));
    m_reload->setToolTip(tr("Look for new plugins and restart the enabled ones."));

    for (int i = 0; i < m_list->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = m_list->topLevelItem(i);
        updateItem(item, m_manager->plugin(item->data(NameColumn, Qt::UserRole).toString()));
    }
    updateDetails();
}

void PluginsPage::rebuild()
{
    m_updating = true;
    m_list->clear();
    for (const Plugin* plugin : m_manager->plugins()) {
        auto* item = new QTreeWidgetItem(m_list);
        item->setData(NameColumn, Qt::UserRole, plugin->id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        updateItem(item, plugin);
    }
    for (int column = NameColumn; column < StatusColumn; column++) {
        m_list->resizeColumnToContents(column);
    }
    m_updating = false;
    updateDetails();
}

void PluginsPage::updateItem(QTreeWidgetItem* item, const Plugin* plugin)
{
    if (!plugin) {
        return;
    }
    const bool updating = m_updating;
    m_updating = true;

    const auto& manifest = plugin->manifest();
    item->setText(NameColumn, manifest ? manifest->name : plugin->id());
    item->setText(IdColumn, plugin->id());
    item->setText(VersionColumn, manifest ? manifest->version : QString());
    item->setCheckState(NameColumn, m_manager->isEnabled(plugin->id()) ? Qt::Checked : Qt::Unchecked);

    switch (plugin->state()) {
        case Plugin::State::Disabled:
            item->setText(StatusColumn, tr("Disabled"));
            item->setIcon(StatusColumn, QIcon());
            break;
        case Plugin::State::Loaded:
            item->setText(StatusColumn, tr("Running"));
            item->setIcon(StatusColumn, QIcon::fromTheme("status-good"));
            break;
        case Plugin::State::Error:
            item->setText(StatusColumn, tr("Error"));
            item->setIcon(StatusColumn, QIcon::fromTheme("status-bad"));
            break;
    }

    m_updating = updating;
}

void PluginsPage::updateDetails()
{
    QTreeWidgetItem* item = m_list->currentItem();
    const Plugin* plugin = item ? m_manager->plugin(item->data(NameColumn, Qt::UserRole).toString()) : nullptr;
    if (!plugin) {
        m_details->setText(m_list->topLevelItemCount() == 0 ? tr("No plugins found. Put each plugin in its own folder "
                                                                 "inside the plugins folder.")
                                                            : QString());
        return;
    }

    QString text = tr("Folder: %1").arg(plugin->directory());
    if (plugin->manifest() && !plugin->manifest()->permissions.isEmpty()) {
        text += '\n' + tr("Permissions: %1").arg(plugin->manifest()->permissions.join(", "));
    }
    if (plugin->state() == Plugin::State::Error) {
        text += '\n' + tr("Error: %1").arg(plugin->error());
    }
    m_details->setText(text);
}

void PluginsPage::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_updating || column != NameColumn) {
        return;
    }
    // The manager reports the outcome through pluginChanged, which updates the item
    m_manager->setEnabled(item->data(NameColumn, Qt::UserRole).toString(), item->checkState(NameColumn) == Qt::Checked);
}

void PluginsPage::onPluginChanged(const QString& id)
{
    for (int i = 0; i < m_list->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = m_list->topLevelItem(i);
        if (item->data(NameColumn, Qt::UserRole).toString() == id) {
            updateItem(item, m_manager->plugin(id));
            if (item == m_list->currentItem()) {
                updateDetails();
            }
            return;
        }
    }
}
