#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include <QDebug>
#include "MultiMC.h"
#include <settings/SettingsObject.h>
#include <Json.h>

#include <hoedown/html.h>
#include <hoedown/document.h>

#include "BuildConfig.h"

UpdateDialog::UpdateDialog(bool hasUpdate, QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog)
{
	ui->setupUi(this);
	auto channel = MMC->settings()->get("UpdateChannel").toString();
	if(hasUpdate)
	{
		ui->label->setText(tr("A new %1 update is available!").arg(channel));
	}
	else
	{
		ui->label->setText(tr("No %1 updates found. You are running the latest version.").arg(channel));
		ui->btnUpdateNow->setHidden(true);
		ui->btnUpdateLater->setText(tr("Close"));
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
	QString url;
	if(channel == "stable")
	{
		url = QString("https://raw.githubusercontent.com/MultiMC/MultiMC5/%1/changelog.md").arg(channel);
		m_changelogType = CHANGELOG_MARKDOWN;
	}
	else
	{
		url = QString("https://api.github.com/repos/MultiMC/MultiMC5/compare/%1...%2").arg(BuildConfig.GIT_COMMIT, channel);
		m_changelogType = CHANGELOG_COMMITS;
	}
	dljob->addNetAction(Net::Download::makeByteArray(QUrl(url), &changelogData));
	connect(dljob.get(), &NetJob::succeeded, this, &UpdateDialog::changelogLoaded);
	connect(dljob.get(), &NetJob::failed, this, &UpdateDialog::changelogFailed);
	dljob->start();
}

/**
 * hoedown wrapper, because dealing with resource lifetime in C is stupid
 */
class HoeDown
{
public:
	class buffer
	{
	public:
		buffer(size_t unit = 4096)
		{
			buf = hoedown_buffer_new(unit);
		}
		~buffer()
		{
			hoedown_buffer_free(buf);
		}
		const char * cstr()
		{
			return hoedown_buffer_cstr(buf);
		}
		void put(QByteArray input)
		{
			hoedown_buffer_put(buf, (uint8_t *) input.data(), input.size());
		}
		const uint8_t * data() const
		{
			return buf->data;
		}
		size_t size() const
		{
			return buf->size;
		}
		hoedown_buffer * buf;
	} ib, ob;
	HoeDown()
	{
		renderer = hoedown_html_renderer_new((hoedown_html_flags) 0,0);
		document = hoedown_document_new(renderer, (hoedown_extensions) 0, 8);
	}
	~HoeDown()
	{
		hoedown_document_free(document);
		hoedown_html_renderer_free(renderer);
	}
	QString process(QByteArray input)
	{
		ib.put(input);
		hoedown_document_render(document, ob.buf, ib.data(), ib.size());
		return ob.cstr();
	}
private:
	hoedown_document * document;
	hoedown_renderer * renderer;
};

QString reprocessMarkdown(QByteArray markdown)
{
	HoeDown hoedown;
	QString output = hoedown.process(markdown);

	// HACK: easier than customizing hoedown
	output.replace(QRegExp("GH-([0-9]+)"), "<a href=\"https://github.com/MultiMC/MultiMC5/issues/\\1\">GH-\\1</a>");
	qDebug() << output;
	return output;
}

QString reprocessCommits(QByteArray json)
{
	auto channel = MMC->settings()->get("UpdateChannel").toString();
	try
	{
		QString result;
		auto document = Json::requireDocument(json);
		auto rootobject = Json::requireObject(document);
		auto status = Json::requireString(rootobject, "status");
		auto diff_url = Json::requireString(rootobject, "html_url");

		auto print_commits = [&]()
		{
			result += "<table cellspacing=0 cellpadding=2 style='border-width: 1px; border-style: solid'>";
			auto commitarray = Json::requireArray(rootobject, "commits");
			for(int i = commitarray.size() - 1; i >= 0; i--)
			{
				const auto & commitval = commitarray[i];
				auto commitobj = Json::requireObject(commitval);
				auto commit_url = Json::requireString(commitobj, "html_url");
				auto commit_info = Json::requireObject(commitobj, "commit");
				auto commit_message = Json::requireString(commit_info, "message");
				auto lines = commit_message.split('\n');
				QRegularExpression regexp("(?<prefix>(GH-(?<issuenr>[0-9]+))|(NOISSUE)|(SCRATCH))? *(?<rest>.*) *");
				auto match = regexp.match(lines.takeFirst(), 0, QRegularExpression::NormalMatch);
				auto issuenr = match.captured("issuenr");
				auto prefix = match.captured("prefix");
				auto rest = match.captured("rest");
				result += "<tr><td>";
				if(issuenr.length())
				{
					result += QString("<a href=\"https://github.com/MultiMC/MultiMC5/issues/%1\">GH-%2</a>").arg(issuenr, issuenr);
				}
				else if(prefix.length())
				{
					result += QString("<a href=\"%1\">%2</a>").arg(commit_url, prefix);
				}
				else
				{
					result += QString("<a href=\"%1\">NOISSUE</a>").arg(commit_url);
				}
				result += "</td>";
				lines.prepend(rest);
				result += "<td><p>" + lines.join("<br />") + "</p></td></tr>";
			}
			result += "</table>";
		};

		if(status == "identical")
		{
			return QObject::tr("<p>There are no code changes between your current version and latest %1.</p>").arg(channel);
		}
		else if(status == "ahead")
		{
			result += QObject::tr("<p>Following commits were added since last update:</p>");
			print_commits();
		}
		else if(status == "diverged")
		{
			auto commit_ahead = Json::requireInteger(rootobject, "ahead_by");
			auto commit_behind = Json::requireInteger(rootobject, "behind_by");
			result += QObject::tr("<p>The update removes %1 commits and adds the following %2:</p>").arg(commit_behind).arg(commit_ahead);
			print_commits();
		}
		result += QObject::tr("<p>You can <a href=\"%1\">look at the changes on github</a>.</p>").arg(diff_url);
		return result;
	}
	catch (JSONValidationError & e)
	{
		qWarning() << "Got an unparseable commit log from github:" << e.what();
		qDebug() << json;
	}
	return QString();
}

void UpdateDialog::changelogLoaded()
{
	QString result;
	switch(m_changelogType)
	{
		case CHANGELOG_COMMITS:
			result = reprocessCommits(changelogData);
			break;
		case CHANGELOG_MARKDOWN:
			result = reprocessMarkdown(changelogData);
			break;
	}
	changelogData.clear();
	ui->changelogBrowser->setHtml(result);
}

void UpdateDialog::changelogFailed(QString reason)
{
	ui->changelogBrowser->setHtml(tr("<p align=\"center\" <span style=\"font-size:22pt;\">Failed to fetch changelog... Error: %1</span></p>").arg(reason));
}

void UpdateDialog::on_btnUpdateLater_clicked()
{
	reject();
}

void UpdateDialog::on_btnUpdateNow_clicked()
{
	done(UPDATE_NOW);
}
