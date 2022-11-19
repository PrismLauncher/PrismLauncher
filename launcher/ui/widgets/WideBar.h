#pragma once

#include <QAction>
#include <QMap>
#include <QMenu>
#include <QToolBar>

#include <memory>

class WideBar : public QToolBar {
    Q_OBJECT

   public:
    explicit WideBar(const QString& title, QWidget* parent = nullptr);
    explicit WideBar(QWidget* parent = nullptr);
    ~WideBar() override = default;

    void addAction(QAction* action);
    void addSeparator();

    void insertSpacer(QAction* action);
    void insertSeparator(QAction* before);
    void insertActionBefore(QAction* before, QAction* action);
    void insertActionAfter(QAction* after, QAction* action);

    QMenu* createContextMenu(QWidget* parent = nullptr, const QString& title = QString());
    void contextMenuEvent(QContextMenuEvent*) override;

   private:
    struct BarEntry {
        enum class Type { None, Action, Separator, Spacer } type = Type::None;
        QAction* bar_action = nullptr;
        QAction* menu_action = nullptr;
    };

    auto getMatching(QAction* act) -> QList<BarEntry>::iterator;

   private:
    QList<BarEntry> m_entries;

    // Menu to toggle visibility from buttons in the bar
    std::unique_ptr<QMenu> m_bar_menu = nullptr;
    enum class MenuState { Fresh, Dirty } m_menu_state = MenuState::Dirty;
};
