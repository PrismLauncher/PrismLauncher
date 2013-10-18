#include "EditNotesDialog.h"
#include "ui_EditNotesDialog.h"
#include "gui/platform.h"

#include <QIcon>
#include <QApplication>

EditNotesDialog::EditNotesDialog( QString notes, QString name, QWidget* parent ) :
    m_instance_notes(notes),
    m_instance_name(name),
    QDialog(parent),
    ui(new Ui::EditNotesDialog)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	ui->noteEditor->setText(notes);
	setWindowTitle(tr("Edit notes of %1").arg(m_instance_name));
	//connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));
}

EditNotesDialog::~EditNotesDialog()
{
    delete ui;
}

QString EditNotesDialog::getText()
{
	return ui->noteEditor->toPlainText();
}
