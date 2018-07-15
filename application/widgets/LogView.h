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
    QAbstractItemModel *m_model = nullptr;
    QTextCharFormat *m_defaultFormat = nullptr;
    bool m_scroll = false;
    bool m_scrolling = false;
};
