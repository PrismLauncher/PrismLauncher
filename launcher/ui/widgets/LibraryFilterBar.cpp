// SPDX-License-Identifier: GPL-3.0-only
#include "LibraryFilterBar.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QSignalBlocker>
#include <QToolButton>

#include <algorithm>

namespace {
// Groups shown as chips before the rest move into a "More" menu
constexpr int s_maxChips = 7;
const char* const s_groupProperty = "_prism_group";
const char* const s_allProperty = "_prism_all_groups";
}  // namespace

LibraryFilterBar::LibraryFilterBar(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);

    m_search = new QLineEdit(this);
    m_search->setObjectName(QStringLiteral("librarySearch"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(160);
    m_search->setMaximumWidth(320);
    layout->addWidget(m_search, 2);
    connect(m_search, &QLineEdit::textChanged, this, &LibraryFilterBar::searchChanged);

    auto* chipArea = new QWidget(this);
    m_chipLayout = new QHBoxLayout(chipArea);
    m_chipLayout->setContentsMargins(0, 0, 0, 0);
    m_chipLayout->setSpacing(4);
    layout->addWidget(chipArea, 3);

    m_chips = new QButtonGroup(this);
    m_chips->setExclusive(false);

    m_sort = new QComboBox(this);
    m_sort->setObjectName(QStringLiteral("librarySort"));
    m_sort->addItem(QString(), QStringLiteral("Name"));
    m_sort->addItem(QString(), QStringLiteral("LastLaunch"));
    m_sort->addItem(QString(), QStringLiteral("Playtime"));
    m_sort->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    layout->addWidget(m_sort);
    connect(m_sort, &QComboBox::currentIndexChanged, this, [this](int index) { emit sortModeChanged(m_sort->itemData(index).toString()); });

    auto* find = new QShortcut(QKeySequence::Find, this);
    connect(find, &QShortcut::activated, this, &LibraryFilterBar::focusSearch);
    auto* clear = new QShortcut(QKeySequence(Qt::Key_Escape), m_search);
    clear->setContext(Qt::WidgetShortcut);
    connect(clear, &QShortcut::activated, m_search, &QLineEdit::clear);

    retranslate();
}

void LibraryFilterBar::setGroups(const QList<GroupCount>& groups, int total)
{
    if (groups == m_groups && total == m_total && !m_chips->buttons().isEmpty()) {
        return;
    }
    m_groups = groups;
    m_total = total;
    rebuildChips();

    const bool stillThere =
        !m_group || std::any_of(m_groups.cbegin(), m_groups.cend(), [this](const GroupCount& g) { return g.first == *m_group; });
    if (!stillThere) {
        selectGroup(std::nullopt);
    }
}

void LibraryFilterBar::rebuildChips()
{
    for (auto* button : m_chips->buttons()) {
        m_chips->removeButton(button);
    }
    while (auto* item = m_chipLayout->takeAt(0)) {
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if (m_groups.size() < 2) {
        // A single group would only repeat "All"
        m_chipLayout->addStretch(1);
        return;
    }
    addChip(tr("All"), m_total, std::nullopt);
    QMenu* more = nullptr;
    int shown = 0;
    for (const auto& [name, count] : m_groups) {
        const QString label = name.isEmpty() ? tr("Ungrouped") : name;
        if (shown < s_maxChips) {
            addChip(label, count, name);
            ++shown;
            continue;
        }
        if (!more) {
            auto* moreChip = new QToolButton(this);
            moreChip->setText(tr("More"));
            moreChip->setAutoRaise(true);
            moreChip->setPopupMode(QToolButton::InstantPopup);
            more = new QMenu(moreChip);
            moreChip->setMenu(more);
            m_chipLayout->addWidget(moreChip);
        }
        auto* action = more->addAction(QStringLiteral("%1 (%2)").arg(label).arg(count));
        connect(action, &QAction::triggered, this, [this, name] { selectGroup(name); });
    }
    m_chipLayout->addStretch(1);
    updateChipStates();
}

QToolButton* LibraryFilterBar::addChip(const QString& label, int count, std::optional<QString> group)
{
    auto* chip = new QToolButton(this);
    chip->setText(QStringLiteral("%1 %2").arg(label).arg(count));
    chip->setCheckable(true);
    chip->setAutoRaise(true);
    chip->setToolButtonStyle(Qt::ToolButtonTextOnly);
    chip->setProperty(s_allProperty, !group.has_value());
    chip->setProperty(s_groupProperty, group.value_or(QString()));
    m_chips->addButton(chip);
    m_chipLayout->addWidget(chip);
    connect(chip, &QToolButton::clicked, this, [this, group] { selectGroup(group); });
    return chip;
}

void LibraryFilterBar::selectGroup(std::optional<QString> group)
{
    const bool changed = group != m_group;
    m_group = group;
    updateChipStates();
    if (changed) {
        emit groupChanged(m_group);
    }
}

void LibraryFilterBar::updateChipStates()
{
    for (auto* button : m_chips->buttons()) {
        const bool all = button->property(s_allProperty).toBool();
        const bool selected = all ? !m_group.has_value() : (m_group && *m_group == button->property(s_groupProperty).toString());
        button->setChecked(selected);
    }
}

void LibraryFilterBar::setSortMode(const QString& mode)
{
    const int index = m_sort->findData(mode);
    if (index >= 0 && index != m_sort->currentIndex()) {
        QSignalBlocker blocker(m_sort);
        m_sort->setCurrentIndex(index);
    }
}

void LibraryFilterBar::focusSearch()
{
    m_search->setFocus(Qt::ShortcutFocusReason);
    m_search->selectAll();
}

QString LibraryFilterBar::searchText() const
{
    return m_search->text();
}

void LibraryFilterBar::retranslate()
{
    m_search->setPlaceholderText(tr("Search instances"));
    m_search->setToolTip(tr("Filter by name, Minecraft version or mod loader (%1)")
                             .arg(QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
    m_sort->setItemText(0, tr("Sort by name"));
    m_sort->setItemText(1, tr("Recently played"));
    m_sort->setItemText(2, tr("Most played"));
    m_sort->setToolTip(tr("Order of instances within each group"));
}

void LibraryFilterBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
        rebuildChips();
    }
    QWidget::changeEvent(event);
}
