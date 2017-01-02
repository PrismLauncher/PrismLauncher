#include "SetupWizard.h"
#include "translations/TranslationsModel.h"
#include <MultiMC.h>
#include <FileSystem.h>
#include <ganalytics.h>

enum Page
{
	Language,
	Java,
	Analytics,
	// Themes,
	// Accounts
};

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListView>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWizard>
#include <QtWidgets/QWizardPage>

class BaseWizardPage : public QWizardPage
{
public:
	explicit BaseWizardPage(QWidget *parent = Q_NULLPTR)
		: QWizardPage(parent)
	{
	}
	virtual ~BaseWizardPage() {};

protected:
	virtual void retranslate() = 0;
	void changeEvent(QEvent * event) override
	{
		if (event->type() == QEvent::LanguageChange)
		{
			retranslate();
		}
		QWizardPage::changeEvent(event);
	}
};

class LanguageWizardPage : public BaseWizardPage
{
	Q_OBJECT;
public:
	explicit LanguageWizardPage(QWidget *parent = Q_NULLPTR)
		: BaseWizardPage(parent)
	{
		setObjectName(QStringLiteral("languagePage"));
		verticalLayout = new QVBoxLayout(this);
		verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
		languageView = new QListView(this);
		languageView->setObjectName(QStringLiteral("languageView"));
		verticalLayout->addWidget(languageView);
		retranslate();

		auto translations = MMC->translations();
		auto index = translations->selectedIndex();
		languageView->setModel(translations.get());
		languageView->setCurrentIndex(index);
		connect(languageView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LanguageWizardPage::languageRowChanged);
	}

	virtual ~LanguageWizardPage()
	{
	};

	bool validatePage() override
	{
		auto settings = MMC->settings();
		auto translations = MMC->translations();
		QString key = translations->data(languageView->currentIndex(), Qt::UserRole).toString();
		settings->set("Language", key);
		return true;
	}

	static bool isRequired()
	{
		auto settings = MMC->settings();
		if (settings->get("Language").toString().isEmpty())
			return true;
		return false;
	}

protected:
	void retranslate() override
	{
		setTitle(QApplication::translate("LanguageWizardPage", "Language", Q_NULLPTR));
		setSubTitle(QApplication::translate("LanguageWizardPage", "Select the language to use in MultiMC", Q_NULLPTR));
	}

protected slots:
	void languageRowChanged(const QModelIndex &current, const QModelIndex &previous)
	{
		if (current == previous)
		{
			return;
		}
		auto translations = MMC->translations();
		QString key = translations->data(current, Qt::UserRole).toString();
		translations->selectLanguage(key);
		translations->updateLanguage(key);
	}

private:
	QVBoxLayout *verticalLayout = nullptr;
	QListView *languageView = nullptr;
};

class AnalyticsWizardPage : public BaseWizardPage
{
	Q_OBJECT;
public:
	explicit AnalyticsWizardPage(QWidget *parent = Q_NULLPTR)
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

	virtual ~AnalyticsWizardPage()
	{
	};

	bool validatePage() override
	{
		auto settings = MMC->settings();
		auto analytics = MMC->analytics();
		auto status = checkBox->isChecked();
		settings->set("AnalyticsSeen", analytics->version());
		settings->set("Analytics", status);
		return true;
	}

	static bool isRequired()
	{
		auto settings = MMC->settings();
		auto analytics = MMC->analytics();
		if(!settings->get("Analytics").toBool())
		{
			return false;
		}
		if(settings->get("AnalyticsSeen").toInt() < analytics->version())
		{
			return true;
		}
		return false;
	}

protected:
	void retranslate() override
	{
		setTitle(QApplication::translate("AnalyticsWizardPage", "Analytics", Q_NULLPTR));
		setSubTitle(QApplication::translate("AnalyticsWizardPage", "We track some anonymous statistics about users.", Q_NULLPTR));
		textBrowser->setHtml(QApplication::translate("AnalyticsWizardPage",
			"<html><body>"
			"<p>MultiMC sends anonymous usage statistics on every start of the application. This helps us decide what platforms and issues to focus on.</p>"
			"<p>The data is processed by Google Analytics, see their <a href=\"https://support.google.com/analytics/answer/6004245?hl=en\">article on the matter</a>.</p>"
			"<p>The following data is collected:</p>"
			"<ul><li>A random unique ID of the MultiMC installation.<br />It is stored in the application settings (multimc.cfg).</li>"
			"<li>Anonymized IP address.<br />Last octet is set to 0 by Google and not stored.</li>"
			"<li>MultiMC version.</li>"
			"<li>Operating system name, version and architecture.</li>"
			"<li>CPU architecture (kernel architecture on linux).</li>"
			"<li>Size of system memory.</li>"
			"<li>Java version, architecture and memory settings.</li></ul>"
			"<p>The analytics will activate on next start, unless you disable them in the settings.</p>"
			"<p>If we change the tracked information, analytics won't be active for the first run and you will see this page again.</p></body></html>", Q_NULLPTR));
		checkBox->setText(QApplication::translate("AnalyticsWizardPage", "Enable Analytics", Q_NULLPTR));
	}
private:
	QVBoxLayout *verticalLayout_3 = nullptr;
	QTextBrowser *textBrowser = nullptr;
	QCheckBox *checkBox = nullptr;
};

SetupWizard::SetupWizard(QWidget *parent) : QWizard(parent)
{
	setObjectName(QStringLiteral("SetupWizard"));
	resize(615, 659);
	// make it ugly everywhere to avoid variability in theming
	setWizardStyle(QWizard::ClassicStyle);
	setOptions(QWizard::NoCancelButton | QWizard::IndependentPages);
	if (LanguageWizardPage::isRequired())
	{
		setPage(Page::Language, new LanguageWizardPage(this));
	}
	if(AnalyticsWizardPage::isRequired())
	{
		setPage(Page::Analytics, new AnalyticsWizardPage(this));
	}
}

void SetupWizard::retranslate()
{
	setButtonText(QWizard::NextButton, tr("Next >"));
	setButtonText(QWizard::BackButton, tr("< Back"));
	setButtonText(QWizard::FinishButton, tr("Finish"));
	setWindowTitle(QApplication::translate("SetupWizard", "MultiMC Quick Setup", Q_NULLPTR));
}

void SetupWizard::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		retranslate();
	}
	QWizard::changeEvent(event);
}

SetupWizard::~SetupWizard()
{
}

/*
bool SetupWizard::javaIsRequired()
{
	QString currentHostName = QHostInfo::localHostName();
	QString oldHostName = MMC->settings()->get("LastHostname").toString();
	if (currentHostName != oldHostName)
	{
		MMC->settings()->set("LastHostname", currentHostName);
		return true;
	}
	QString currentJavaPath = MMC->settings()->get("JavaPath").toString();
	QString actualPath = FS::ResolveExecutable(currentJavaPath);
	if (actualPath.isNull())
	{
		return true;
	}
	return false;
}
*/

bool SetupWizard::isRequired()
{
	if (LanguageWizardPage::isRequired())
		return true;
	if (AnalyticsWizardPage::isRequired())
		return true;
	return false;
}

/*
void MainWindow::checkSetDefaultJava()
{
	qDebug() << "Java path needs resetting, showing Java selection dialog...";

	JavaInstallPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, false);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.selectedVersion())
		java = std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
	else
	{
		CustomMessageBox::selectable(this, tr("Invalid version selected"), tr("You didn't select a valid Java version, so MultiMC will "
																				"select the default. "
																				"You can change this in the settings dialog."),
										QMessageBox::Warning)
			->show();

		JavaUtils ju;
		java = ju.GetDefaultJava();
	}
	if (java)
	{
		MMC->settings()->set("JavaPath", java->path);
	}
	else
		MMC->settings()->set("JavaPath", QString("java"));
}
*/

#include "SetupWizard.moc"
