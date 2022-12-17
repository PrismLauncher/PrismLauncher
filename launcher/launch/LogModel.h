#pragma once

#include <QAbstractListModel>
#include <QString>
#include "MessageLevel.h"

class LogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit LogModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

    void append(MessageLevel::Enum, QString line);
    void clear();

    void suspend(bool suspend);
    bool suspended();

    QString toPlainText();

    int getMaxLines();
    void setMaxLines(int maxLines);
    void setStopOnOverflow(bool stop);
    void setOverflowMessage(const QString & overflowMessage);

    void setLineWrap(bool state);
    bool wrapLines() const;

    enum Roles
    {
        LevelRole = Qt::UserRole
    };

private /* types */:
    struct entry
    {
        MessageLevel::Enum level;
        QString line;
    };

private: /* data */
    QVector <entry> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines = 1000;
    // first line in the circular buffer
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine = 0;
    // number of lines occupied in the circular buffer
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines = 0;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stopOnOverflow = false;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_overflowMessage = "OVERFLOW";
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspended = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lineWrap = true;

private:
    Q_DISABLE_COPY(LogModel)
};
