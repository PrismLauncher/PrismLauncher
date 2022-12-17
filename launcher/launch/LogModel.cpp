#include "LogModel.h"

LogModel::LogModel(QObject *parent):QAbstractListModel(parent)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content.resize(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines);
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines;
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines)
        return QVariant();

    auto row = index.row();
    auto realRow = (row + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines;
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[realRow].line;
    }
    if(role == LevelRole)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[realRow].level;
    }

    return QVariant();
}

void LogModel::append(MessageLevel::Enum level, QString line)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspended)
    {
        return;
    }
    int lineNum = (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines;
    // overflow
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines)
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stopOnOverflow)
        {
            // nothing more to do, the buffer is full
            return;
        }
        beginRemoveRows(QModelIndex(), 0, 0);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine = (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + 1) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines --;
        endRemoveRows();
    }
    else if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines - 1 && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stopOnOverflow)
    {
        level = MessageLevel::Fatal;
        line = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_overflowMessage;
    }
    beginInsertRows(QModelIndex(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines ++;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[lineNum].level = level;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[lineNum].line = line;
    endInsertRows();
}

void LogModel::suspend(bool suspend)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspended = suspend;
}

bool LogModel::suspended()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_suspended;
}

void LogModel::clear()
{
    beginResetModel();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine = 0;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines = 0;
    endResetModel();
}

QString LogModel::toPlainText()
{
    QString out;
    out.reserve(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines * 80);
    for(int i = 0; i < hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines; i++)
    {
        QString & line = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + i) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines].line;
        out.append(line + '\n');
    }
    out.squeeze();
    return out;
}

void LogModel::setMaxLines(int maxLines)
{
    // no-op
    if(maxLines == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines)
    {
        return;
    }
    // if it all still fits in the buffer, just resize it
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines < hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines = maxLines;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content.resize(maxLines);
        return;
    }
    // otherwise, we need to reorganize the data because it crosses the wrap boundary
    QVector<entry> newContent;
    newContent.resize(maxLines);
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines <= maxLines)
    {
        // if it all fits in the new buffer, just copy it over
        for(int i = 0; i < hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines; i++)
        {
            newContent[i] = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + i) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines];
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content.swap(newContent);
    }
    else
    {
        // if it doesn't fit, part of the data needs to be thrown away (the oldest log messages)
        int lead = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines - maxLines;
        beginRemoveRows(QModelIndex(), 0, lead - 1);
        for(int i = 0; i < maxLines; i++)
        {
            newContent[i] = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content[(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine + lead + i) % hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines];
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numLines = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_content.swap(newContent);
        endRemoveRows();
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_firstLine = 0;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines = maxLines;
}

int LogModel::getMaxLines()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxLines;
}

void LogModel::setStopOnOverflow(bool stop)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stopOnOverflow = stop;
}

void LogModel::setOverflowMessage(const QString& overflowMessage)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_overflowMessage = overflowMessage;
}

void LogModel::setLineWrap(bool state)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lineWrap != state)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lineWrap = state;
    }
}

bool LogModel::wrapLines() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lineWrap;
}
