// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginsPage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
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

QString PluginsPage::describePermission(const QString& permission)
{
    if (permission == "instances.read") {
        return tr("See your instances and what happens with them (launches, whether the game is running).");
    }
    if (permission == "fs.instance") {
        return tr("Read and change the files in your instances' game folders (worlds, mods, options).");
    }
    if (permission == "fs.plugins") {
        return tr("Read and change the files of every plugin, including their code.");
    }
    if (permission == "game.log") {
        return tr("Read the game's log while it runs. It may include chat messages and server addresses.");
    }
    if (permission == "launch.hooks") {
        return tr("Run its own steps when an instance is launched, and stop the launch.");
    }
    if (permission == "net") {
        return tr("Connect to the internet.");
    }
    if (permission == "ui") {
        return tr("Add buttons and menus to the launcher, and show notifications.");
    }
    if (permission == "unrestricted") {
        return tr("Full access to your computer: read, change and delete any of your files, and run programs.");
    }
    return permission;
}

bool PluginsPage::askForPermissions(const Plugin* plugin, const QStringList& missing)
{
    const auto& manifest = *plugin->manifest();
    QString text = tr("<b>%1</b> asks for these permissions:").arg(manifest.name.toHtmlEscaped()) + "<ul>";
    for (const QString& permission : manifest.permissions) {
        const QString description = describePermission(permission).toHtmlEscaped();
        // Highlight what was not granted before, e.g. after an update
        text += missing.contains(permission) && missing.size() != manifest.permissions.size()
                    ? "<li><b>" + tr("New:") + "</b> " + description + "</li>"
                    : "<li>" + description + "</li>";
    }
    text += "</ul>";

    const bool unrestricted = manifest.permissions.contains("unrestricted");
    if (unrestricted) {
        text += "<p><b>" +
                tr("Warning: this plugin asks for full access. It could damage your system or steal your data, and the launcher "
                   "cannot protect you from it. Only enable it if you completely trust its author.") +
                "</b></p>";
    }

    QMessageBox box(unrestricted ? QMessageBox::Warning : QMessageBox::Question, tr("Enable plugin"), text, QMessageBox::Cancel, this);
    box.setTextFormat(Qt::RichText);
    QPushButton* enable = box.addButton(tr("Enable"), QMessageBox::AcceptRole);
    // A full access plugin needs a deliberate click
    if (unrestricted) {
        box.setDefaultButton(QMessageBox::Cancel);
    } else {
        box.setDefaultButton(enable);
    }
    box.exec();
    return box.clickedButton() == enable;
}

void PluginsPage::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_updating || column != NameColumn) {
        return;
    }
    const QString id = item->data(NameColumn, Qt::UserRole).toString();
    const bool enabled = item->checkState(NameColumn) == Qt::Checked;
    if (enabled) {
        const Plugin* plugin = m_manager->plugin(id);
        const QStringList missing = m_manager->missingPermissions(id);
        if (plugin && plugin->manifest() && !missing.isEmpty()) {
            if (!askForPermissions(plugin, missing)) {
                updateItem(item, plugin);
                return;
            }
            m_manager->grantPermissions(id);
        }
    }
    // The manager reports the outcome through pluginChanged, which updates the item
    m_manager->setEnabled(id, enabled);
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
