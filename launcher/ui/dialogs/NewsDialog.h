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

    QHash<QString, NewsEntryPtr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden = false;
};
