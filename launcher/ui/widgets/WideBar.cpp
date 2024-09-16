#include "WideBar.h"

#include <QContextMenuEvent>
#include <QCryptographicHash>
#include <QToolButton>

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

    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &QToolBar::customContextMenuRequested, this, &WideBar::showVisibilityMenu);
}

WideBar::WideBar(QWidget* parent) : QToolBar(parent)
{
    setFloatable(false);
    setMovable(false);

    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &QToolBar::customContextMenuRequested, this, &WideBar::showVisibilityMenu);
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
    auto iter = std::find_if(m_entries.begin(), m_entries.end(), [act](BarEntry const& entry) { return entry.menu_action == act; });

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

static void copyAction(QAction* from, QAction* to)
{
    Q_ASSERT(from);
    Q_ASSERT(to);

    to->setText(from->text());
    to->setIcon(from->icon());
    to->setToolTip(from->toolTip());
}

void WideBar::showVisibilityMenu(QPoint const& position)
{
    if (!m_bar_menu) {
        m_bar_menu = std::make_unique<QMenu>(this);
        m_bar_menu->setTearOffEnabled(true);
    }

    if (m_menu_state == MenuState::Dirty) {
        for (auto* old_action : m_bar_menu->actions())
            old_action->deleteLater();

        m_bar_menu->clear();

        m_bar_menu->addActions(m_context_menu_actions);

        m_bar_menu->addSeparator()->setText(tr("Customize toolbar actions"));

        for (auto& entry : m_entries) {
            if (entry.type != BarEntry::Type::Action)
                continue;

            auto act = new QAction();
            copyAction(entry.menu_action, act);

            act->setCheckable(true);
            act->setChecked(entry.bar_action->isVisible());

            connect(act, &QAction::toggled, entry.bar_action, [this, &entry](bool toggled) {
                entry.bar_action->setVisible(toggled);

                // NOTE: This is needed so that disabled actions get reflected on the button when it is made visible.
                static_cast<ActionButton*>(widgetForAction(entry.bar_action))->actionChanged();
            });

            m_bar_menu->addAction(act);
        }

        m_menu_state = MenuState::Fresh;
    }

    m_bar_menu->popup(mapToGlobal(position));
}

void WideBar::addContextMenuAction(QAction* action)
{
    m_context_menu_actions.append(action);
}

[[nodiscard]] QByteArray WideBar::getVisibilityState() const
{
    QByteArray state;

    for (auto const& entry : m_entries) {
        if (entry.type != BarEntry::Type::Action)
            continue;

        state.append(entry.bar_action->isVisible() ? '1' : '0');
    }

    state.append(',');
    state.append(getHash());

    return state;
}

void WideBar::setVisibilityState(QByteArray&& state)
{
    auto split = state.split(',');

    auto bits = split.first();
    auto hash = split.last();

    // If the actions changed, we better not try to load the old one to avoid unwanted hiding
    if (!checkHash(hash))
        return;

    qsizetype i = 0;
    for (auto& entry : m_entries) {
        if (entry.type != BarEntry::Type::Action)
            continue;
        if (i == bits.size())
            break;

        entry.bar_action->setVisible(bits.at(i++) == '1');

        // NOTE: This is needed so that disabled actions get reflected on the button when it is made visible.
        static_cast<ActionButton*>(widgetForAction(entry.bar_action))->actionChanged();
    }
}

QByteArray WideBar::getHash() const
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    for (auto const& entry : m_entries) {
        if (entry.type != BarEntry::Type::Action)
            continue;
        hash.addData(entry.menu_action->text().toLatin1());
    }

    return hash.result().toBase64();
}

bool WideBar::checkHash(QByteArray const& old_hash) const
{
    return old_hash == getHash();
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
