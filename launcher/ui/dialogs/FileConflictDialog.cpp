#include "FileConflictDialog.h"
#include "ui_FileConflictDialog.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPushButton>

#include "Application.h"

FileConflictDialog::FileConflictDialog(QString source, QString destination, bool move, QWidget* parent)
    : QDialog(parent), ui(new Ui::FileConflictDialog), m_result(Result::Cancel)
{
    ui->setupUi(this);

    QLocale locale;

    // Setup buttons
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &FileConflictDialog::cancel);
    if (move) {
        setWindowTitle("File conflict while moving files");

        auto chooseSourceButton = ui->buttonBox->addButton(tr("Keep source"), QDialogButtonBox::DestructiveRole);
        chooseSourceButton->setIcon(APPLICATION->getThemedIcon("delete"));
        connect(chooseSourceButton, &QPushButton::clicked, this, &FileConflictDialog::chooseSource);

        auto chooseDestinationButton = ui->buttonBox->addButton(tr("Keep destination"), QDialogButtonBox::DestructiveRole);
        chooseDestinationButton->setIcon(APPLICATION->getThemedIcon("delete"));
        connect(chooseDestinationButton, &QPushButton::clicked, this, &FileConflictDialog::chooseDestination);
    } else {
        setWindowTitle("File conflict while copying files");

        auto chooseSourceButton = ui->buttonBox->addButton(tr("Overwrite destination"), QDialogButtonBox::DestructiveRole);
        chooseSourceButton->setIcon(APPLICATION->getThemedIcon("delete"));
        connect(chooseSourceButton, &QPushButton::clicked, this, &FileConflictDialog::chooseSource);

        auto chooseDestinationButton = ui->buttonBox->addButton(tr("Skip"), QDialogButtonBox::DestructiveRole);
        connect(chooseDestinationButton, &QPushButton::clicked, this, &FileConflictDialog::chooseDestination);
    }

    // Setup info
    QFileInfo sourceInfo(source);
    ui->sourceInfoLabel->setText(tr("<b>Name:</b> %1<br/><b>Size:</b> %2<br/><b>Last modified:</b> %3")
                                     .arg(source, sourceInfo.isDir() ? "-" : locale.formattedDataSize(sourceInfo.size()),
                                          sourceInfo.lastModified().toString(locale.dateTimeFormat())));

    QFileInfo destinationInfo(destination);
    ui->destinationInfoLabel->setText(tr("<b>Name:</b> %1<br/><b>Size:</b> %2<br/><b>Last modified:</b> %3")
                                          .arg(destination,
                                               destinationInfo.isDir() ? "-" : locale.formattedDataSize(destinationInfo.size()),
                                               destinationInfo.lastModified().toString(locale.dateTimeFormat())));
}

FileConflictDialog::~FileConflictDialog()
{
    delete ui;
}

FileConflictDialog::Result FileConflictDialog::execWithResult()
{
    exec();
    return m_result;
}

FileConflictDialog::Result FileConflictDialog::getResult() const
{
    return m_result;
}

void FileConflictDialog::chooseSource()
{
    m_result = Result::ChooseSource;
    accept();
}

void FileConflictDialog::chooseDestination()
{
    m_result = Result::ChooseDestination;
    accept();
}

void FileConflictDialog::cancel()
{
    m_result = Result::Cancel;
    reject();
}
