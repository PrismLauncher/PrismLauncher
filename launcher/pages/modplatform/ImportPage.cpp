#include "ImportPage.h"
#include "ui_ImportPage.h"

#include "Application.h"
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

ImportPage::ImportPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), ui(new Ui::ImportPage), dialog(dialog)
{
    ui->setupUi(this);
    ui->modpackEdit->setValidator(new UrlValidator(ui->modpackEdit));
    connect(ui->modpackEdit, &QLineEdit::textChanged, this, &ImportPage::updateState);
}

ImportPage::~ImportPage()
{
    delete ui;
}

bool ImportPage::shouldDisplay() const
{
    return true;
}

void ImportPage::openedImpl()
{
    updateState();
}

void ImportPage::updateState()
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
                dialog->setSuggestedIcon("default");
            }
        }
        else
        {
            if(input.endsWith("?client=y")) {
                input.chop(9);
                input.append("/file");
                url = QUrl::fromUserInput(input);
            }
            // hook, line and sinker.
            QFileInfo fi(url.fileName());
            dialog->setSuggestedPack(fi.completeBaseName(), new InstanceImportTask(url));
            dialog->setSuggestedIcon("default");
        }
    }
    else
    {
        dialog->setSuggestedPack();
    }
}

void ImportPage::setUrl(const QString& url)
{
    ui->modpackEdit->setText(url);
    updateState();
}

void ImportPage::on_modpackBtn_clicked()
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


QUrl ImportPage::modpackUrl() const
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
