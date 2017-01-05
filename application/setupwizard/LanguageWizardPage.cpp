#include "LanguageWizardPage.h"
#include <MultiMC.h>
#include <translations/TranslationsModel.h>

#include <QVBoxLayout>
#include <QListView>

LanguageWizardPage::LanguageWizardPage(QWidget *parent)
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

LanguageWizardPage::~LanguageWizardPage()
{
}

bool LanguageWizardPage::wantsRefreshButton()
{
	return true;
}

void LanguageWizardPage::refresh()
{
	auto translations = MMC->translations();
	translations->downloadIndex();
}

bool LanguageWizardPage::validatePage()
{
	auto settings = MMC->settings();
	auto translations = MMC->translations();
	QString key = translations->data(languageView->currentIndex(), Qt::UserRole).toString();
	settings->set("Language", key);
	return true;
}

bool LanguageWizardPage::isRequired()
{
	auto settings = MMC->settings();
	if (settings->get("Language").toString().isEmpty())
		return true;
	return false;
}

void LanguageWizardPage::retranslate()
{
	setTitle(QApplication::translate("LanguageWizardPage", "Language", Q_NULLPTR));
	setSubTitle(QApplication::translate("LanguageWizardPage", "Select the language to use in MultiMC", Q_NULLPTR));
}

void LanguageWizardPage::languageRowChanged(const QModelIndex &current, const QModelIndex &previous)
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
