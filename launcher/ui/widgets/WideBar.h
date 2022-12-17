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
    void showVisibilityMenu(const QPoint&);

    // Ideally we would use a QBitArray for this, but it doesn't support string conversion,
    // so using it in settings is very messy.

    [[nodiscard]] QByteArray getVisibilityState() const;
    void setVisibilityState(QByteArray&&);

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
    QList<BarEntry> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries;

    // Menu to toggle visibility from buttons in the bar
    std::unique_ptr<QMenu> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar_menu = nullptr;
    enum class MenuState { Fresh, Dirty } hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_menu_state = MenuState::Dirty;
};
