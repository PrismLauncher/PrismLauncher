#ifndef BROWSERDIALOG_H
#define BROWSERDIALOG_H

#include <QDialog>

namespace Ui {
class BrowserDialog;
}

class BrowserDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BrowserDialog(QWidget *parent = 0);
    ~BrowserDialog();

    void load(const QUrl &url);

    void setPageTitleInWindowTitle(bool enable);
    bool pageTitleInWindowTitle(void) { return m_pageTitleInWindowTitle; }

    void setWindowTitleFormat(QString format);
    QString windowTitleFormat(void) { return m_windowTitleFormat; }
    
private:
    Ui::BrowserDialog *ui;

    bool m_pageTitleInWindowTitle;
    QString m_windowTitleFormat;

    void refreshWindowTitle(void);

private slots:
    void on_btnBack_clicked(void);
    void on_btnForward_clicked(void);
    void on_webView_urlChanged(const QUrl &url);
    void on_webView_titleChanged(const QString &title);
};

#endif // BROWSERDIALOG_H
