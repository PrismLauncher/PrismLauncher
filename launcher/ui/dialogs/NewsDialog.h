#pragma once

#include <QDialog>
#include <QHash>

#include "news/NewsEntry.h"

namespace Ui {
class NewsDialog;
}

class NewsDialog : public QDialog {
    Q_OBJECT

   public:
    NewsDialog(QList<NewsEntryPtr> entries, QWidget* parent = nullptr);
    ~NewsDialog();

   public slots:
    void toggleArticleList();

   private slots:
    void selectedArticleChanged(const QString& new_title);

   private:
    Ui::NewsDialog* ui;

    QHash<QString, NewsEntryPtr> m_entries;
    bool m_article_list_hidden = false;
};
