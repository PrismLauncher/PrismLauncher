// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"

class Plugin;
class PluginManager;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

// Lists the plugins found in the plugins folder and lets the user enable or disable them. Changes apply immediately.
class PluginsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit PluginsPage(QWidget* parent = nullptr);
    ~PluginsPage() override = default;

    QString displayName() const override { return tr("Plugins"); }
    QIcon icon() const override { return QIcon::fromTheme("plugins"); }
    QString id() const override { return "plugins"; }

    void retranslate() override;

   private:
    void rebuild();
    void updateItem(QTreeWidgetItem* item, const Plugin* plugin);
    void updateDetails();
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onPluginChanged(const QString& id);
    // Shows the permissions the plugin requests and returns whether the user accepted them
    bool askForPermissions(const Plugin* plugin, const QStringList& missing);
    static QString describePermission(const QString& permission);

    PluginManager* m_manager;
    QLabel* m_intro;
    QTreeWidget* m_list;
    QLabel* m_details;
    QPushButton* m_openFolder;
    QPushButton* m_reload;
    // Set while the list is filled, so the checkboxes do not enable or disable plugins
    bool m_updating = false;
};
