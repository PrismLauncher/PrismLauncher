#include "MultiMCPage.h"
#include "ui_MultiMCPage.h"

#include "MultiMC.h"
#include "dialogs/NewInstanceDialog.h"
#include <QFileDialog>
#include <QValidator>
#include <InstanceImportTask.h>

class UrlValidator : public QValidator
{
public:
    using QValidator::QValidator;

    State validate(QString &in, int &pos) const
    {
        const QUrl url(in);
        if (url.isValid() && !url.isRelative() && !url.isEmpty())
        {
            return Acceptable;
        }
        else if (QFile::exists(in))
        {
            return Acceptable;
        }
        else
        {
            return Intermediate;
        }
    }
};

MultiMCPage::MultiMCPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::MultiMCPage), dialog(dialog)
{
    ui->setupUi(this);
    ui->modpackEdit->setValidator(new UrlValidator(ui->modpackEdit));
    connect(ui->modpackEdit, &QLineEdit::textChanged, this, &MultiMCPage::updateState);
}

MultiMCPage::~MultiMCPage()
{
    delete ui;
}

bool MultiMCPage::shouldDisplay() const
{
    return true;
}

void MultiMCPage::openedImpl()
{
    updateState();
}

void MultiMCPage::updateState()
{
    if(!isOpened)
    {
        return;
    }
    if(ui->modpackEdit->hasAcceptableInput())
    {
        QString input = ui->modpackEdit->text();
        auto url = QUrl::fromUserInput(input);
        if(url.isLocalFile())
        {
            // FIXME: actually do some validation of what's inside here... this is fake AF
            QFileInfo fi(input);
            if(fi.exists() && fi.suffix() == "zip")
            {
                QFileInfo fi(url.fileName());
                dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(url));
            }
        }
        else
        {
            // hook, line and sinker.
            QFileInfo fi(url.fileName());
            dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(url));
        }
    }
    else
    {
        dialog->setSuggestedPack();
    }
}

void MultiMCPage::setUrl(const QString& url)
{
    ui->modpackEdit->setText(url);
    updateState();
}

void MultiMCPage::on_modpackBtn_clicked()
{
    const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Choose modpack"), modpackUrl(), tr("Zip (*.zip)"));
    if (url.isValid())
    {
        if (url.isLocalFile())
        {
            ui->modpackEdit->setText(url.toLocalFile());
        }
        else
        {
            ui->modpackEdit->setText(url.toString());
        }
    }
}


QUrl MultiMCPage::modpackUrl() const
{
    const QUrl url(ui->modpackEdit->text());
    if (url.isValid() && !url.isRelative() && !url.host().isEmpty())
    {
        return url;
    }
    else
    {
        return QUrl::fromLocalFile(ui->modpackEdit->text());
    }
}
