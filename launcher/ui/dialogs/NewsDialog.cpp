#include "NewsDialog.h"
#include "ui_NewsDialog.h"

NewsDialog::NewsDialog(QList<NewsEntryPtr> entries, QWidget* parent) : QDialog(parent), ui(new Ui::NewsDialog())
{
    ui->setupUi(this);

    for (auto entry : entries) {
        ui->articleListWidget->addItem(entry->title);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.insert(entry->title, entry);
    }

    connect(ui->articleListWidget, &QListWidget::currentTextChanged, this, &NewsDialog::selectedArticleChanged);
    connect(ui->toggleListButton, &QPushButton::clicked, this, &NewsDialog::toggleArticleList);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden = ui->articleListWidget->isHidden();

    auto first_item = ui->articleListWidget->item(0);
    first_item->setSelected(true);

    auto article_entry = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.constFind(first_item->text()).value();
    ui->articleTitleLabel->setText(QString("<a href='%1'>%2</a>").arg(article_entry->link, first_item->text()));

    ui->currentArticleContentBrowser->setText(article_entry->content);
    ui->currentArticleContentBrowser->flush();
}

NewsDialog::~NewsDialog()
{
    delete ui;
}

void NewsDialog::selectedArticleChanged(const QString& new_title)
{
    auto const& article_entry = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entries.constFind(new_title).value();

    ui->articleTitleLabel->setText(QString("<a href='%1'>%2</a>").arg(article_entry->link, new_title));

    ui->currentArticleContentBrowser->setText(article_entry->content);
    ui->currentArticleContentBrowser->flush();
}

void NewsDialog::toggleArticleList()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden = !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden;

    ui->articleListWidget->setHidden(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden);

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_article_list_hidden)
        ui->toggleListButton->setText(tr("Show article list"));
    else
        ui->toggleListButton->setText(tr("Hide article list"));
}
