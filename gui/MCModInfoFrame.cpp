#include "MCModInfoFrame.h"
#include "ui_MCModInfoFrame.h"

void handleModInfoUpdate(Mod &m, MCModInfoFrame *frame)
{
	QString missing = "<p><span style=\" font-style:italic; color:#4a4a4a;\">Missing from mcmod.info</span></p>";

	QString name = m.name();
	if(name.isEmpty()) name = missing;
	QString description = m.description();
	if(description.isEmpty()) description = missing;
	QString authors = m.authors();
	if(authors.isEmpty()) authors = missing;
	QString credits = m.credits();
	if(credits.isEmpty()) credits = missing;
	QString website = m.homeurl();
	if(website.isEmpty()) website = missing;
	else website = "<a href=\"" + website + "\">" + website + "</a>";

	frame->setName("<p><span style=\" font-size:9pt; font-weight:600;\">" + name + "</span></p>");
	frame->setDescription(description);
	frame->setAuthors(authors);
	frame->setCredits(credits);
	frame->setWebsite(website);
}

MCModInfoFrame::MCModInfoFrame(QWidget *parent) :
	QFrame(parent),
	ui(new Ui::MCModInfoFrame)
{
	ui->setupUi(this);
}

MCModInfoFrame::~MCModInfoFrame()
{
	delete ui;
}

void MCModInfoFrame::setName(QString name)
{
	ui->label_Name->setText(name);
	ui->label_Name->setToolTip(name);
}

void MCModInfoFrame::setDescription(QString description)
{
	ui->label_Description->setText(description);
	ui->label_Description->setToolTip(description);
}

void MCModInfoFrame::setAuthors(QString authors)
{
	ui->label_Authors->setText(authors);
	ui->label_Authors->setToolTip(authors);
}

void MCModInfoFrame::setCredits(QString credits)
{
	ui->label_Credits->setText(credits);
	ui->label_Credits->setToolTip(credits);
}

void MCModInfoFrame::setWebsite(QString website)
{
	ui->label_Website->setText(website);
}
