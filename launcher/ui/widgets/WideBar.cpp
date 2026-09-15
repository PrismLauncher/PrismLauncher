#include "WideBar.h"

#include <QContextMenuEvent>
#include <QCryptographicHash>
#include <QToolButton>

#include <algorithm>

class ActionButton : public QToolButton {
    Q_OBJECT
   public:
    ActionButton(QAction* action, QWidget* parent = nullptr, bool use_default_action = false)
        : QToolButton(parent), m_action(action), m_use_default_action(use_default_action)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        // workaround for breeze and breeze forks
        setProperty("_kde_toolButton_alignment", Qt::AlignLeft);

        if (m_use_default_action) {
            setDefaultAction(action);
        } else {
            connect(this, &ActionButton::clicked, action, &QAction::trigger);
        }
        connect(action, &QAction::changed, this, &ActionButton::actionChanged);

        actionChanged();
    };
   public slots:
    void actionChanged()
    {
        setEnabled(m_action->isEnabled());
        // better pop up mode
        if (m_action->menu()) {
            setPopupMode(QToolButton::MenuButtonPopup);
        }
        if (!m_use_default_action) {
            setMenu(m_action->menu());
            setChecked(m_action->isChecked());
            setCheckable(m_action->isCheckable());
            setText(m_action->text());
            setIcon(m_action->icon());
            setToolTip(m_action->toolTip());
            setHidden(!m_action->isVisible());
        }
        setFocusPolicy(Qt::NoFocus);
    }

   private:
    QAction* m_action;
    bool m_use_default_action;
};

WideBar::WideBar(const QString& title, QWidget* parent) : QToolBar(title, parent)
{
    setFloatable(false);
    setMovable(false);
}

WideBar::WideBar(QWidget* parent) : QToolBar(parent)
{
    setFloatable(false);
    setMovable(false);
}

void WideBar::addAction(QAction* action)
{
    BarEntry entry;
    entry.bar_action = addWidget(new ActionButton(action, this, m_use_default_action));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    m_entries.push_back(entry);

    m_menu_state = MenuState::Dirty;
}

void WideBar::addSeparator()
{
    BarEntry entry;
    entry.bar_action = QToolBar::addSeparator();
    entry.type = BarEntry::Type::Separator;

    m_entries.push_back(entry);
}

auto WideBar::getMatching(QAction* act) -> QList<BarEntry>::iterator
{
    auto iter = std::ranges::find_if(m_entries, [act](const BarEntry& entry) { return entry.menu_action == act; });

    return iter;
}

void WideBar::insertActionBefore(QAction* before, QAction* action)
{
    auto iter = getMatching(before);
    if (iter == m_entries.end())
        return;

    BarEntry entry;
    entry.bar_action = insertWidget(iter->bar_action, new ActionButton(action, this, m_use_default_action));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    m_entries.insert(iter, entry);

    m_menu_state = MenuState::Dirty;
}

void WideBar::insertActionAfter(QAction* after, QAction* action)
{
    auto iter = getMatching(after);
    if (iter == m_entries.end())
        return;

    iter++;
    // the action to insert after is present
    // however, the element after it isn't valid
    if (iter == m_entries.end()) {
        // append the action instead of inserting it
        addAction(action);
        return;
    }

    BarEntry entry;
    entry.bar_action = insertWidget(iter->bar_action, new ActionButton(action, this, m_use_default_action));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    m_entries.insert(iter, entry);

    m_menu_state = MenuState::Dirty;
}

void WideBar::insertWidgetBefore(QAction* before, QWidget* widget)
{
    auto iter = getMatching(before);
    if (iter == m_entries.end())
        return;

    insertWidget(iter->bar_action, widget);
}

void WideBar::insertSpacer(QAction* action)
{
    auto iter = getMatching(action);
    if (iter == m_entries.end())
        return;

    auto* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    BarEntry entry;
    entry.bar_action = insertWidget(iter->bar_action, spacer);
    entry.type = BarEntry::Type::Spacer;
    m_entries.insert(iter, entry);
}

void WideBar::insertSeparator(QAction* before)
{
    auto iter = getMatching(before);
    if (iter == m_entries.end())
        return;

    BarEntry entry;
    entry.bar_action = QToolBar::insertSeparator(iter->bar_action);
    entry.type = BarEntry::Type::Separator;

    m_entries.insert(iter, entry);
}

QMenu* WideBar::createContextMenu(QWidget* parent, const QString& title)
{
    auto* contextMenu = new QMenu(title, parent);
    for (auto& item : m_entries) {
        switch (item.type) {
            default:
            case BarEntry::Type::None:
                break;
            case BarEntry::Type::Separator:
            case BarEntry::Type::Spacer:
                contextMenu->addSeparator();
                break;
            case BarEntry::Type::Action:
                contextMenu->addAction(item.menu_action);
                break;
        }
    }
    return contextMenu;
}

void WideBar::removeAction(QAction* action)
{
    auto iter = getMatching(action);
    if (iter == m_entries.end())
        return;

    iter->bar_action->setVisible(false);
    removeAction(iter->bar_action);
    m_entries.erase(iter);
}

#include "WideBar.moc"
