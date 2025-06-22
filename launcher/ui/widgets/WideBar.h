#pragma once

#include <QAction>
#include <QMap>
#include <QMenu>
#include <QToolBar>

#include <memory>

class WideBar : public QToolBar {
    Q_OBJECT
    // Why: so we can enable / disable alt shortcuts in toolbuttons
    // with toolbuttons using setDefaultAction, theres no alt shortcuts
    Q_PROPERTY(bool useDefaultAction MEMBER m_use_default_action)

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
    void insertWidgetBefore(QAction* before, QWidget* widget);

    QMenu* createContextMenu(QWidget* parent = nullptr, const QString& title = QString());
    void showVisibilityMenu(const QPoint&);

    void addContextMenuAction(QAction* action);

    // Ideally we would use a QBitArray for this, but it doesn't support string conversion,
    // so using it in settings is very messy.

    [[nodiscard]] QByteArray getVisibilityState() const;
    void setVisibilityState(QByteArray&&);

    void removeAction(QAction* action);

   private:
    struct BarEntry {
        enum class Type { None, Action, Separator, Spacer } type = Type::None;
        QAction* bar_action = nullptr;
        QAction* menu_action = nullptr;
    };

    auto getMatching(QAction* act) -> QList<BarEntry>::iterator;

    /** Used to distinguish between versions of the WideBar with different actions */
    [[nodiscard]] QByteArray getHash() const;
    [[nodiscard]] bool checkHash(QByteArray const&) const;

   private:
    QList<BarEntry> m_entries;

    QList<QAction*> m_context_menu_actions;

    bool m_use_default_action = false;

    // Menu to toggle visibility from buttons in the bar
    std::unique_ptr<QMenu> m_bar_menu = nullptr;
    enum class MenuState { Fresh, Dirty } m_menu_state = MenuState::Dirty;
};
