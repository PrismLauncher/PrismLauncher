// SPDX-License-Identifier: GPL-3.0-only
#include "OverviewPage.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include "Application.h"
#include "InstanceList.h"
#include "MMCTime.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"

OverviewPage::OverviewPage(MinecraftInstance* instance, QWidget* parent) : QWidget(parent), m_instance(instance)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    // Icon and name
    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setPixmap(APPLICATION->icons()->getIcon(m_instance->iconKey()).pixmap(64, 64));
    layout->addWidget(m_iconLabel);

    m_nameLabel = new QLabel(m_instance->name(), this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setTextFormat(Qt::PlainText);
    m_nameLabel->setWordWrap(true);
    QFont nameFont = font();
    nameFont.setBold(true);
    nameFont.setPointSizeF(nameFont.pointSizeF() + 2.0);
    m_nameLabel->setFont(nameFont);
    layout->addWidget(m_nameLabel);

    m_factsLabel = new QLabel(this);
    m_factsLabel->setAlignment(Qt::AlignCenter);
    m_factsLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_factsLabel);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    m_statsLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_statsLabel);

    // Quick links into the instance's content
    auto* linksRow = new QWidget(this);
    auto* linksLayout = new QHBoxLayout(linksRow);
    linksLayout->setContentsMargins(0, 0, 0, 0);
    linksLayout->setSpacing(6);
    const auto addLink = [this, linksRow, linksLayout](const char* text, const char* iconName, const char* pageId) {
        auto* button = new QToolButton(linksRow);
        button->setText(tr(text));
        button->setIcon(QIcon::fromTheme(QString::fromLatin1(iconName)));
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QToolButton::clicked, this, [this, pageId = QString::fromLatin1(pageId)] {
            if (m_container)
                m_container->selectPage(pageId);
        });
        linksLayout->addWidget(button);
        m_links.append({ button, text });
    };
    addLink("Mods", "loadermods", "mods");
    addLink("Worlds", "worlds", "worlds");
    addLink("Screenshots", "screenshots", "screenshots");
    addLink("Servers", "server", "servers");
    linksLayout->addStretch(1);
    layout->addWidget(linksRow);

    // Pack updates only exist for managed packs
    m_packButton = new QToolButton(this);
    m_packButton->setIcon(QIcon::fromTheme(QStringLiteral("checkupdate")));
    m_packButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_packButton->setAutoRaise(true);
    m_packButton->setFocusPolicy(Qt::NoFocus);
    connect(m_packButton, &QToolButton::clicked, this, [this] {
        if (m_container)
            m_container->selectPage(QStringLiteral("managed_pack"));
    });
    layout->addWidget(m_packButton);

    // Notes preview
    m_notesLabel = new QLabel(this);
    m_notesLabel->setWordWrap(true);
    m_notesLabel->setTextFormat(Qt::PlainText);
    layout->addWidget(m_notesLabel);

    m_notesButton = new QToolButton(this);
    m_notesButton->setIcon(QIcon::fromTheme(QStringLiteral("notes")));
    m_notesButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_notesButton->setAutoRaise(true);
    m_notesButton->setFocusPolicy(Qt::NoFocus);
    connect(m_notesButton, &QToolButton::clicked, this, [this] {
        if (m_container)
            m_container->selectPage(QStringLiteral("notes"));
    });
    layout->addWidget(m_notesButton);

    layout->addStretch(1);

    retranslateUi();
    refresh();
}

void OverviewPage::retranslateUi()
{
    m_packButton->setText(tr("Pack updates"));
    m_notesButton->setText(tr("Open notes"));
    for (const auto& link : m_links) {
        link.button->setText(tr(link.text));
    }
}

void OverviewPage::refresh()
{
    // The same facts the library tiles show, straight from the cached instance summary
    QString version;
    QString loader;
    QString source;
    if (auto* instances = APPLICATION->instances()) {
        const QModelIndex index = instances->getInstanceIndexById(m_instance->id());
        if (index.isValid()) {
            version = index.data(InstanceList::MinecraftVersionRole).toString();
            loader = index.data(InstanceList::ModLoaderRole).toString();
            source = index.data(InstanceList::PackSourceRole).toString();
        }
    }

    QStringList facts;
    if (!version.isEmpty())
        facts << tr("Minecraft %1").arg(version);
    if (!loader.isEmpty())
        facts << loader;
    if (!source.isEmpty())
        facts << source;
    m_factsLabel->setText(facts.join(QStringLiteral(" · ")));

    // Play stats
    QStringList stats;
    const qint64 last = m_instance->lastLaunch();
    if (last > 0) {
        const double agoSeconds = (QDateTime::currentMSecsSinceEpoch() - last) / 1000.0;
        stats << tr("Last played %1 ago").arg(Time::humanReadableDuration(agoSeconds));
    } else {
        stats << tr("Never played");
    }
    if (const int64_t played = m_instance->totalTimePlayed(); played > 0) {
        stats << tr("Total time: %1").arg(Time::prettifyDuration(played));
    }
    m_statsLabel->setText(stats.join(QStringLiteral(" · ")));

    // Notes preview
    const QString notes = m_instance->notes().trimmed();
    const bool hasNotes = !notes.isEmpty();
    m_notesLabel->setVisible(hasNotes);
    m_notesButton->setVisible(hasNotes);
    if (hasNotes) {
        QString preview = notes;
        preview.replace(QLatin1Char('\n'), QLatin1Char(' '));
        if (preview.size() > 400) {
            preview.truncate(397);
            preview += QChar(0x2026);  // …
        }
        m_notesLabel->setText(preview);
    }

    m_packButton->setVisible(m_instance->isManagedPack());
}
