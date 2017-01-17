#include "AnalyticsWizardPage.h"
#include <MultiMC.h>

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QCheckBox>

#include <ganalytics.h>
#include <BuildConfig.h>

AnalyticsWizardPage::AnalyticsWizardPage(QWidget *parent)
	: BaseWizardPage(parent)
{
	setObjectName(QStringLiteral("analyticsPage"));
	verticalLayout_3 = new QVBoxLayout(this);
	verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
	textBrowser = new QTextBrowser(this);
	textBrowser->setObjectName(QStringLiteral("textBrowser"));
	textBrowser->setAcceptRichText(false);
	textBrowser->setOpenExternalLinks(true);
	verticalLayout_3->addWidget(textBrowser);

	checkBox = new QCheckBox(this);
	checkBox->setObjectName(QStringLiteral("checkBox"));
	checkBox->setChecked(true);
	verticalLayout_3->addWidget(checkBox);
	retranslate();
}

AnalyticsWizardPage::~AnalyticsWizardPage()
{
}

bool AnalyticsWizardPage::validatePage()
{
	auto settings = MMC->settings();
	auto analytics = MMC->analytics();
	auto status = checkBox->isChecked();
	settings->set("AnalyticsSeen", analytics->version());
	settings->set("Analytics", status);
	return true;
}

bool AnalyticsWizardPage::isRequired()
{
	if(BuildConfig.ANALYTICS_ID.isEmpty())
	{
		return false;
	}
	auto settings = MMC->settings();
	auto analytics = MMC->analytics();
	if (!settings->get("Analytics").toBool())
	{
		return false;
	}
	if (settings->get("AnalyticsSeen").toInt() < analytics->version())
	{
		return true;
	}
	return false;
}

void AnalyticsWizardPage::retranslate()
{
	setTitle(tr("Analytics"));
	setSubTitle(tr("We track some anonymous statistics about users."));
	textBrowser->setHtml(tr(
		"<html><body>"
		"<p>MultiMC sends anonymous usage statistics on every start of the application. This helps us decide what platforms and issues to focus on.</p>"
		"<p>The data is processed by Google Analytics, see their <a href=\"https://support.google.com/analytics/answer/6004245?hl=en\">article on the "
		"matter</a>.</p>"
		"<p>The following data is collected:</p>"
		"<ul><li>A random unique ID of the MultiMC installation.<br />It is stored in the application settings (multimc.cfg).</li>"
		"<li>Anonymized (partial) IP address.</li>"
		"<li>MultiMC version.</li>"
		"<li>Operating system name, version and architecture.</li>"
		"<li>CPU architecture (kernel architecture on linux).</li>"
		"<li>Size of system memory.</li>"
		"<li>Java version, architecture and memory settings.</li></ul>"
		"<p>If we change the tracked information, you will see this page again.</p></body></html>"));
	checkBox->setText(tr("Enable Analytics"));
}
