// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QList>
#include <QWidget>

#include "ui/pages/BasePage.h"

class MinecraftInstance;
class QLabel;
class QToolButton;

// First page of the instance editor: what the instance is, how it plays and where its content lives.
class OverviewPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit OverviewPage(MinecraftInstance* instance, QWidget* parent = nullptr);
    ~OverviewPage() override = default;

    QString id() const override { return QStringLiteral("overview"); }
    QString displayName() const override { return tr("Overview"); }
    QIcon icon() const override { return QIcon::fromTheme(QStringLiteral("minecraft")); }
    void openedImpl() override { refresh(); }
    void retranslate() override { retranslateUi(); }

   private:
    void refresh();
    void retranslateUi();

    struct LinkButton {
        QToolButton* button = nullptr;
        const char* text;
    };

    MinecraftInstance* m_instance = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_factsLabel = nullptr;
    QLabel* m_statsLabel = nullptr;
    QLabel* m_notesLabel = nullptr;
    QToolButton* m_packButton = nullptr;
    QToolButton* m_notesButton = nullptr;
    QList<LinkButton> m_links;
};
