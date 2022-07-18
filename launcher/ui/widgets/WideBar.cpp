#include "WideBar.h"
#include <QToolButton>
#include <QMenu>

class ActionButton : public QToolButton
{
    Q_OBJECT
public:
    ActionButton(QAction * action, QWidget * parent = 0) : QToolButton(parent), m_action(action) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(action, &QAction::changed, this, &ActionButton::actionChanged);
        connect(this, &ActionButton::clicked, action, &QAction::trigger);
        actionChanged();
    };
private slots:
    void actionChanged() {
        setEnabled(m_action->isEnabled());
        setChecked(m_action->isChecked());
        setCheckable(m_action->isCheckable());
        setText(m_action->text());
        setIcon(m_action->icon());
        setToolTip(m_action->toolTip());
        setHidden(!m_action->isVisible());
        setFocusPolicy(Qt::NoFocus);
    }
private:
    QAction * m_action;
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

struct WideBar::BarEntry {
    enum Type {
        None,
        Action,
        Separator,
        Spacer
    } type = None;
    QAction *qAction = nullptr;
    QAction *wideAction = nullptr;
};


WideBar::~WideBar()
{
    for(auto *iter: m_entries) {
        delete iter;
    }
}

void WideBar::addAction(QAction* action)
{
    auto entry = new BarEntry();
    entry->qAction = addWidget(new ActionButton(action, this));
    entry->wideAction = action;
    entry->type = BarEntry::Action;
    m_entries.push_back(entry);
}

void WideBar::addSeparator()
{
    auto entry = new BarEntry();
    entry->qAction = QToolBar::addSeparator();
    entry->type = BarEntry::Separator;
    m_entries.push_back(entry);
}

auto WideBar::getMatching(QAction* act) -> QList<BarEntry*>::iterator
{
    auto iter = std::find_if(m_entries.begin(), m_entries.end(), [act](BarEntry * entry) {
        return entry->wideAction == act;
    });
    
    return iter;
}

void WideBar::insertActionBefore(QAction* before, QAction* action){
    auto iter = getMatching(before);
    if(iter == m_entries.end())
        return;

    auto entry = new BarEntry();
    entry->qAction = insertWidget((*iter)->qAction, new ActionButton(action, this));
    entry->wideAction = action;
    entry->type = BarEntry::Action;
    m_entries.insert(iter, entry);
}

void WideBar::insertActionAfter(QAction* after, QAction* action){
    auto iter = getMatching(after);
    if(iter == m_entries.end())
        return;

    auto entry = new BarEntry();
    entry->qAction = insertWidget((*(iter+1))->qAction, new ActionButton(action, this));
    entry->wideAction = action;
    entry->type = BarEntry::Action;
    m_entries.insert(iter + 1, entry);
}

void WideBar::insertSpacer(QAction* action)
{
    auto iter = getMatching(action);
    if(iter == m_entries.end())
        return;

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto entry = new BarEntry();
    entry->qAction = insertWidget((*iter)->qAction, spacer);
    entry->type = BarEntry::Spacer;
    m_entries.insert(iter, entry);
}

void WideBar::insertSeparator(QAction* before)
{
    auto iter = getMatching(before);
    if(iter == m_entries.end())
        return;

    auto entry = new BarEntry();
    entry->qAction = QToolBar::insertSeparator(before);
    entry->type = BarEntry::Separator;
    m_entries.insert(iter, entry);
}

QMenu * WideBar::createContextMenu(QWidget *parent, const QString & title)
{
    QMenu *contextMenu = new QMenu(title, parent);
    for(auto & item: m_entries) {
        switch(item->type) {
            default:
            case BarEntry::None:
                break;
            case BarEntry::Separator:
            case BarEntry::Spacer:
                contextMenu->addSeparator();
                break;
            case BarEntry::Action:
                contextMenu->addAction(item->wideAction);
                break;
        }
    }
    return contextMenu;
}

#include "WideBar.moc"
