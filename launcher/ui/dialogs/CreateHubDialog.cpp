// SPDX-License-Identifier: GPL-3.0-only
#include "CreateHubDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CreateHubDialog::CreateHubDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Add an instance"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("new")));

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    auto* header = new QLabel(tr("Choose a start"), this);
    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSizeF(headerFont.pointSizeF() + 2.0);
    header->setFont(headerFont);
    layout->addWidget(header);

    const auto makeChoice = [this, layout](const QString& title, const QString& description, const char* iconName, Action action) {
        auto* button = new QPushButton(title + QLatin1Char('\n') + description, this);
        button->setIcon(QIcon::fromTheme(QString::fromLatin1(iconName)));
        button->setIconSize(QSize(32, 32));
        button->setMinimumHeight(72);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(button, &QPushButton::clicked, this, [this, action] {
            m_action = action;
            accept();
        });
        layout->addWidget(button);
    };
    makeChoice(tr("Start fresh"), tr("Minecraft version plus an optional loader, with release, snapshot and old versions as filters."),
               "minecraft", Action::StartFresh);
    makeChoice(tr("Browse modpacks"), tr("One search across Modrinth, CurseForge, FTB, ATLauncher and Technic."), "modrinth",
               Action::BrowsePacks);
    makeChoice(tr("Import"), tr("Import a modpack from a .zip or .mrpack file on your disk."), "launcher", Action::Import);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
