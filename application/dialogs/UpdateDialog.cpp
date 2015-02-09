#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include "Platform.h"
#include <QDebug>
#include "MultiMC.h"
#include <settings/SettingsObject.h>

UpdateDialog::UpdateDialog(bool hasUpdate, QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	auto channel = MMC->settings()->get("UpdateChannel").toString();
	if(hasUpdate)
	{
		ui->label->setText(tr("A new %1 update is available!").arg(channel));
	}
	else
	{
		ui->label->setText(tr("No %1 updates found. You are running the latest version.").arg(channel));
		ui->btnUpdateNow->setDisabled(true);
		ui->btnUpdateOnExit->setDisabled(true);
	}
	loadChangelog();
}

UpdateDialog::~UpdateDialog()
{
}

void UpdateDialog::loadChangelog()
{
	auto channel = MMC->settings()->get("UpdateChannel").toString();
	dljob.reset(new NetJob("Changelog"));
	auto url = QString("https://raw.githubusercontent.com/MultiMC/MultiMC5/%1/changelog.md").arg(channel);
	changelogDownload = ByteArrayDownload::make(QUrl(url));
	dljob->addNetAction(changelogDownload);
	connect(dljob.get(), &NetJob::succeeded, this, &UpdateDialog::changelogLoaded);
	connect(dljob.get(), &NetJob::failed, this, &UpdateDialog::changelogFailed);
	dljob->start();
}

// TODO: this will be replaced.
QString reprocessMarkdown(QString markdown)
{
	QString htmlData;
	QTextStream html(&htmlData);
	auto lines = markdown.split(QRegExp("[\r]?[\n]"),QString::KeepEmptyParts);
	enum
	{
		BASE,
		LIST1,
		LIST2
	}state = BASE;
	html << "<html>";
	int i = 0;
	auto procLine = [&](QString line) -> QString
	{
		// [GitHub issues](https://github.com/MultiMC/MultiMC5/issues)
		line.replace(QRegExp("\\[([^\\]]+)\\]\\(([^\\)]+)\\)"), "<a href=\"\\2\">\\1</a>");
		return line;
	};
	for(auto line: lines)
	{
		if(line.isEmpty())
		{
			// html << "<br />\n";
		}
		else switch (state)
		{
			case BASE:
				if(line.startsWith("##"))
				{
					html << "<h2>" << procLine(line.mid(2)) << "</h2>\n";
				}
				else if(line.startsWith("#"))
				{
					html << "<h1>" << procLine(line.mid(1)) << "</h1>\n";
				}
				else if(line.startsWith("- "))
				{
					state = LIST1;
					html << "<ul>\n";
					html << "<li>" << procLine(line.mid(2)) << "</li>\n";
				}
				else qCritical() << "Invalid input on line " << i << ": " << line;
				break;
			case LIST1:
				if(line.startsWith("##"))
				{
					state = BASE;
					html << "</ul>\n";
					html << "<h2>" << procLine(line.mid(2)) << "</h2>\n";
				}
				else if(line.startsWith("#"))
				{
					state = BASE;
					html << "</ul>\n";
					html << "<h1>" << procLine(line.mid(1)) << "</h1>\n";
				}
				else if(line.startsWith("- "))
				{
					html << "<li>" << procLine(line.mid(2)) << "</li>\n";
				}
				else if(line.startsWith("  - "))
				{
					state = LIST2;
					html << "<ul>\n";
					html << "<li>" << procLine(line.mid(4)) << "</li>\n";
				}
				else qCritical() << "Invalid input on line " << i << ": " << line;
				break;
			case LIST2:
				if(line.startsWith("##"))
				{
					state = BASE;
					html << "</ul>\n";
					html << "</ul>\n";
					html << "<h2>" << procLine(line.mid(2)) << "</h2>\n";
				}
				else if(line.startsWith("#"))
				{
					state = BASE;
					html << "</ul>\n";
					html << "</ul>\n";
					html << "<h1>" << procLine(line.mid(1)) << "</h1>\n";
				}
				else if(line.startsWith("- "))
				{
					state = LIST1;
					html << "</ul>\n";
					html << "<li>" << procLine(line.mid(2)) << "</li>\n";
				}
				else if(line.startsWith("  - "))
				{
					html << "<li>" << procLine(line.mid(4)) << "</li>\n";
				}
				else qCritical() << "Invalid input on line " << i << ": " << line;
				break;
		}
		i++;
	}
	if(state == LIST2)
	{
		html << "</ul>\n";
		state = LIST1;
	}
	if(state == LIST1)
	{
		html << "</ul>\n";
		state = BASE;
	}
	if (state != BASE)
	{
		qCritical() << "Reprocessing markdown didn't end in a final state!";
	}
	html << "</html>\n";
	qDebug() << htmlData;
	return htmlData;
}

void UpdateDialog::changelogLoaded()
{
	auto rawMarkdown = QString::fromUtf8(changelogDownload->m_data);
	auto html = reprocessMarkdown(rawMarkdown);
	ui->changelogBrowser->setHtml(html);
}

void UpdateDialog::changelogFailed()
{
	ui->changelogBrowser->setHtml(tr("<p align=\"center\" <span style=\"font-size:22pt;\">Failed to fetch changelog...</span></p>"));
}

void UpdateDialog::on_btnUpdateLater_clicked()
{
	reject();
}

void UpdateDialog::on_btnUpdateNow_clicked()
{
	done(UPDATE_NOW);
}

void UpdateDialog::on_btnUpdateOnExit_clicked()
{
	done(UPDATE_ONEXIT);
}
