#pragma once

#include <QAbstractListModel>
#include <QRegularExpression>
#include <QString>

#include "MessageLevel.h"

class LogColorCache;

class LogModel : public QAbstractListModel {
    Q_OBJECT

   public:
    explicit LogModel(QObject* parent = nullptr);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void append(MessageLevel::Enum, QString line);

    void clear();

    void suspend(bool suspend);
    bool suspended();

    QString toPlainText();

    /** Returns a vector of line numbers, beginning index, and end index that match the given text.
     *  If 'use_regex' is true, consider the text as a regex expression instead.
     */
    [[nodiscard]] QVector<std::tuple<int, int, int>> search(QString, bool use_regex) const;

    int getMaxLines();
    void setMaxLines(int maxLines);
    void setStopOnOverflow(bool stop);
    void setOverflowMessage(const QString& overflowMessage);

    enum Roles { LevelRole = Qt::UserRole };

   private /* types */:
    struct entry {
        QString line;
        MessageLevel::Enum level;
    };

   private: /* data */
    QVector<entry> m_content;
    int m_maxLines = 1000;
    // first line in the circular buffer
    int m_firstLine = 0;
    // number of lines occupied in the circular buffer
    int m_numLines = 0;

    bool m_stopOnOverflow = false;
    QString m_overflowMessage = "OVERFLOW";

    bool m_suspended = false;

   private:
    Q_DISABLE_COPY(LogModel)
};
