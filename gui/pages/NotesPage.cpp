#include "NotesPage.h"
#include "ui_NotesPage.h"

QString NotesPage::displayName() const
{
	return tr("Notes");
}

QIcon NotesPage::icon() const
{
	return QIcon::fromTheme("news");
}

QString NotesPage::id() const
{
	return "notes";
}

NotesPage::NotesPage(BaseInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::NotesPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->noteEditor->setText(m_inst->notes());
}

NotesPage::~NotesPage()
{
	delete ui;
}

bool NotesPage::apply()
{
	m_inst->setNotes(ui->noteEditor->toPlainText());
	return true;
}
