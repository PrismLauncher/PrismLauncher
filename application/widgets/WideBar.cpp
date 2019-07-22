#include "WideBar.h"
#include <QToolButton>

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

void WideBar::addAction(QAction* action)
{
    auto actionButton = new ActionButton(action, this);
    auto newAction = addWidget(actionButton);
    m_actionMap[action] = newAction;
}

void WideBar::insertSpacer(QAction* action)
{
    if(!m_actionMap.contains(action)) {
        return;
    }

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    insertWidget(m_actionMap[action], spacer);
}

#include "WideBar.moc"
