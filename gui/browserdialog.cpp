#include "browserdialog.h"
#include "ui_browserdialog.h"

#include <QtWebKit/QWebHistory>

BrowserDialog::BrowserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BrowserDialog),
    m_pageTitleInWindowTitle(true),
    m_windowTitleFormat("%1")
{
    ui->setupUi(this);
    ui->webView->setPage(new QWebPage());
    refreshWindowTitle();
    resize(800, 600);
}

BrowserDialog::~BrowserDialog()
{
    delete ui;
}

// Navigation Buttons
void BrowserDialog::on_btnBack_clicked()
{
    ui->webView->back();
}

void BrowserDialog::on_btnForward_clicked()
{
    ui->webView->forward();
}

void BrowserDialog::on_webView_urlChanged(const QUrl &url)
{
    Q_UNUSED(url);
    qDebug("urlChanged");
    ui->btnBack->setEnabled(ui->webView->history()->canGoBack());
    ui->btnForward->setEnabled(ui->webView->history()->canGoForward());
}

// Window Title Magic
void BrowserDialog::refreshWindowTitle()
{
    qDebug("refreshTitle");
    if (m_pageTitleInWindowTitle)
        setWindowTitle(m_windowTitleFormat.arg(ui->webView->title()));
    else
        setWindowTitle(m_windowTitleFormat);
}

void BrowserDialog::setPageTitleInWindowTitle(bool enable)
{
    m_pageTitleInWindowTitle = enable;
    refreshWindowTitle();
}

void BrowserDialog::setWindowTitleFormat(QString format)
{
    m_windowTitleFormat = format;
    refreshWindowTitle();
}

void BrowserDialog::on_webView_titleChanged(const QString &title)
{
    qDebug("titleChanged");
    if (m_pageTitleInWindowTitle)
        setWindowTitle(m_windowTitleFormat.arg(title));
}

// Public access Methods
void BrowserDialog::load(const QUrl &url)
{
    qDebug("load");
    ui->webView->setUrl(url);
}
