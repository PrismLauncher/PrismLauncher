#include "LogModel.h"

#include <QDebug>

LogModel::LogModel(QObject* parent) : QAbstractListModel(parent)
{
    m_content.resize(m_maxLines);
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "line";
    roles[LevelRole] = "level";
    return roles;
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_numLines;
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_numLines)
        return {};

    auto row = index.row();
    auto realRow = (row + m_firstLine) % m_maxLines;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return m_content.at(realRow).line;
    if (role == LevelRole)
        return m_content.at(realRow).level;

    return {};
}

void LogModel::append(MessageLevel::Enum level, QString line)
{
    if (m_suspended)
        return;

    // overflow
    if (m_numLines == m_maxLines) {
        if (m_stopOnOverflow) {
            // nothing more to do, the buffer is full
            return;
        }

        beginRemoveRows(QModelIndex(), 0, 0);
        m_firstLine = (m_firstLine + 1) % m_maxLines;
        m_numLines--;
        endRemoveRows();
    } else if (m_numLines == m_maxLines - 1 && m_stopOnOverflow) {
        level = MessageLevel::Fatal;
        line = m_overflowMessage;
    }

    // If the level is still undetermined, try to guess it.
    if (level == MessageLevel::StdErr || level == MessageLevel::StdOut || level == MessageLevel::Unknown)
        level = MessageLevel::guessLevel(line, level);

    int lineNum = (m_firstLine + m_numLines) % m_maxLines;
    entry line_entry{ line, level };

    beginInsertRows(QModelIndex(), m_numLines, m_numLines);
    m_content[lineNum] = line_entry;
    m_numLines++;
    endInsertRows();
}

void LogModel::suspend(bool suspend)
{
    m_suspended = suspend;
}

bool LogModel::suspended()
{
    return m_suspended;
}

void LogModel::clear()
{
    beginResetModel();
    m_firstLine = 0;
    m_numLines = 0;
    endResetModel();
}

QString LogModel::toPlainText()
{
    QString out;
    out.reserve(m_numLines * 80);
    for (int i = 0; i < m_numLines; i++) {
        QString& line = m_content[(m_firstLine + i) % m_maxLines].line;
        out.append(line + '\n');
    }
    out.squeeze();
    return out;
}

QVector<std::tuple<int, int, int>> LogModel::search(QString text_to_search, bool use_regex) const
{
    QVector<std::tuple<int, int, int>> matches;

    if (use_regex) {
        QRegularExpression regex{ text_to_search };
        regex.optimize();

        if (!regex.isValid()) {
            qCritical() << "Error in regex pattern:" << regex.errorString();
            return {};
        }

        for (int i = 0; i < m_numLines; i++) {
            auto matches_on_line = regex.globalMatch(m_content.at(i).line);
            while (matches_on_line.hasNext()) {
                auto match = matches_on_line.next();
                for (int k = 0; k <= match.lastCapturedIndex(); k++)
                    matches.append(std::make_tuple(i + 1, match.capturedStart(k), match.capturedEnd(k)));
            }
        }
    } else {
        for (int i = 0; i < m_numLines; i++) {
            auto line = m_content.at(i).line;
            int begin = -1;
            do {
                begin = line.indexOf(text_to_search, begin + 1);
                if (begin >= 0)
                    matches.append(std::make_tuple(i + 1, begin, begin + text_to_search.length()));
            } while (begin >= 0);
        }
    }

    return matches;
}

void LogModel::setMaxLines(int maxLines)
{
    // no-op
    if (maxLines == m_maxLines) {
        return;
    }
    // if it all still fits in the buffer, just resize it
    if (m_firstLine + m_numLines < m_maxLines) {
        m_maxLines = maxLines;
        m_content.resize(maxLines);
        return;
    }
    // otherwise, we need to reorganize the data because it crosses the wrap boundary
    QVector<entry> newContent;
    newContent.resize(maxLines);
    if (m_numLines <= maxLines) {
        // if it all fits in the new buffer, just copy it over
        for (int i = 0; i < m_numLines; i++) {
            newContent[i] = m_content[(m_firstLine + i) % m_maxLines];
        }
        m_content.swap(newContent);
    } else {
        // if it doesn't fit, part of the data needs to be thrown away (the oldest log messages)
        int lead = m_numLines - maxLines;
        beginRemoveRows(QModelIndex(), 0, lead - 1);
        for (int i = 0; i < maxLines; i++) {
            newContent[i] = m_content[(m_firstLine + lead + i) % m_maxLines];
        }
        m_numLines = m_maxLines;
        m_content.swap(newContent);
        endRemoveRows();
    }
    m_firstLine = 0;
    m_maxLines = maxLines;
}

int LogModel::getMaxLines()
{
    return m_maxLines;
}

void LogModel::setStopOnOverflow(bool stop)
{
    m_stopOnOverflow = stop;
}

void LogModel::setOverflowMessage(const QString& overflowMessage)
{
    m_overflowMessage = overflowMessage;
}
