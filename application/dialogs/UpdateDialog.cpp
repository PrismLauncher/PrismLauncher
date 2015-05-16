#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include "Platform.h"
#include <QDebug>
#include "MultiMC.h"
#include <settings/SettingsObject.h>

#include <hoedown/html.h>
#include <hoedown/document.h>

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

void UpdateDialog::changelogLoaded()
{
	auto html = reprocessMarkdown(changelogDownload->m_data);
	ui->changelogBrowser->setHtml(html);
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

void UpdateDialog::on_btnUpdateOnExit_clicked()
{
	done(UPDATE_ONEXIT);
}
