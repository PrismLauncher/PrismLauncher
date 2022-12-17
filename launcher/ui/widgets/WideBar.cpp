#include "WideBar.h"

#include <QContextMenuEvent>
#include <QCryptographicHash>
#include <QToolButton>

class ActionButton : public QToolButton {
    Q_OBJECT
   public:
    ActionButton(QAction* action, QWidget* parent = nullptr) : QToolButton(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action(action)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        connect(action, &QAction::changed, this, &ActionButton::actionChanged);
        connect(this, &ActionButton::clicked, action, &QAction::trigger);

        actionChanged();
    };
   public slots:
    void actionChanged()
    {
        setEnabled(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->isEnabled());
        setChecked(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->isChecked());
        setCheckable(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->isCheckable());
        setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->text());
        setIcon(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->icon());
        setToolTip(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->toolTip());
        setHidden(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action->isVisible());
        setFocusPolicy(Qt::NoFocus);
    }

   private:
    QAction* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_action;
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
    entry.bar_action = addWidget(new ActionButton(action, this));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.push_back(entry);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state = MenuState::Dirty;
}

void WideBar::addSeparator()
{
    BarEntry entry;
    entry.bar_action = QToolBar::addSeparator();
    entry.type = BarEntry::Type::Separator;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.push_back(entry);
}

auto WideBar::getMatching(QAction* act) -> QList<BarEntry>::iterator
{
    auto iter = std::find_if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.begin(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.end(), [act](BarEntry const& entry) { return entry.menu_action == act; });

    return iter;
}

void WideBar::insertActionBefore(QAction* before, QAction* action)
{
    auto iter = getMatching(before);
    if (iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.end())
        return;

    BarEntry entry;
    entry.bar_action = insertWidget(iter->bar_action, new ActionButton(action, this));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.insert(iter, entry);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state = MenuState::Dirty;
}

void WideBar::insertActionAfter(QAction* after, QAction* action)
{
    auto iter = getMatching(after);
    if (iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.end())
        return;

    BarEntry entry;
    entry.bar_action = insertWidget((iter + 1)->bar_action, new ActionButton(action, this));
    entry.menu_action = action;
    entry.type = BarEntry::Type::Action;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.insert(iter + 1, entry);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state = MenuState::Dirty;
}

void WideBar::insertSpacer(QAction* action)
{
    auto iter = getMatching(action);
    if (iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.end())
        return;

    auto* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    BarEntry entry;
    entry.bar_action = insertWidget(iter->bar_action, spacer);
    entry.type = BarEntry::Type::Spacer;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.insert(iter, entry);
}

void WideBar::insertSeparator(QAction* before)
{
    auto iter = getMatching(before);
    if (iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.end())
        return;

    BarEntry entry;
    entry.bar_action = QToolBar::insertSeparator(before);
    entry.type = BarEntry::Type::Separator;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.insert(iter, entry);
}

QMenu* WideBar::createContextMenu(QWidget* parent, const QString& title)
{
    auto* contextMenu = new QMenu(title, parent);
    for (auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries) {
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
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu = std::make_unique<QMenu>(this);

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state == MenuState::Dirty) {
        for (auto* old_action : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu->actions())
            old_action->deleteLater();

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu->clear();

        for (auto& entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries) {
            if (entry.type != BarEntry::Type::Action)
                continue;

            auto act = new QAction();
            copyAction(entry.menu_action, act);

            act->setCheckable(true);
            act->setChecked(entry.bar_action->isVisible());

            connect(act, &QAction::toggled, entry.bar_action, [this, &entry](bool toggled){
                entry.bar_action->setVisible(toggled);

                // NOTE: This is needed so that disabled actions get reflected on the button when it is made visible.
                static_cast<ActionButton*>(widgetForAction(entry.bar_action))->actionChanged();
            });

            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu->addAction(act);
        }

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state = MenuState::Fresh;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu->popup(mapToGlobal(position));
}

[[nodiscard]] QByteArray WideBar::getVisibilityState() const
{
    QByteArray state;

    for (auto const& entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries) {
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
    for (auto& entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries) {
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
    for (auto const& entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries) {
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


#include "WideBar.moc"
