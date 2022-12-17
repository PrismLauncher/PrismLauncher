#pragma once
#include <QPlainTextEdit>
#include <QAbstractItemView>

class QAbstractItemModel;

class LogView: public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogView(QWidget *parent = nullptr);
    virtual ~LogView();

    virtual void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

public slots:
    void setWordWrap(bool wrapping);
    void findNext(const QString & what, bool reverse);
    void scrollToBottom();

protected slots:
    void repopulate();
    // note: this supports only appending
    void rowsInserted(const QModelIndex &parent, int first, int last);
    void rowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    // note: this supports only removing from front
    void rowsRemoved(const QModelIndex &parent, int first, int last);
    void modelDestroyed(QObject * model);

protected:
    QAbstractItemModel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model = nullptr;
    QTextCharFormat *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_defaultFormat = nullptr;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scroll = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_scrolling = false;
};
