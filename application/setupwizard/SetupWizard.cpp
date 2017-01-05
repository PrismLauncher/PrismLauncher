#include "SetupWizard.h"

#include "LanguageWizardPage.h"
#include "JavaWizardPage.h"
#include "AnalyticsWizardPage.h"

#include "translations/TranslationsModel.h"
#include <MultiMC.h>
#include <FileSystem.h>
#include <ganalytics.h>

#include <QAbstractButton>

enum Page
{
	Language,
	Java,
	Analytics,
};

SetupWizard::SetupWizard(QWidget *parent) : QWizard(parent)
{
	setObjectName(QStringLiteral("SetupWizard"));
	resize(615, 659);
	// make it ugly everywhere to avoid variability in theming
	setWizardStyle(QWizard::ClassicStyle);
	setOptions(QWizard::NoCancelButton | QWizard::IndependentPages | QWizard::HaveCustomButton1);

	retranslate();

	connect(this, &QWizard::currentIdChanged, this, &SetupWizard::pageChanged);

	if (LanguageWizardPage::isRequired())
	{
		setPage(Page::Language, new LanguageWizardPage(this));
	}
	if (JavaWizardPage::isRequired())
	{
		setPage(Page::Java, new JavaWizardPage(this));
	}
	if(AnalyticsWizardPage::isRequired())
	{
		setPage(Page::Analytics, new AnalyticsWizardPage(this));
	}
}

void SetupWizard::retranslate()
{
	setButtonText(QWizard::NextButton, tr("&Next >"));
	setButtonText(QWizard::BackButton, tr("< &Back"));
	setButtonText(QWizard::FinishButton, tr("&Finish"));
	setButtonText(QWizard::CustomButton1, tr("&Refresh"));
	setWindowTitle(QApplication::translate("SetupWizard", "MultiMC Quick Setup", Q_NULLPTR));
}

BaseWizardPage * SetupWizard::getBasePage(int id)
{
	if(id == -1)
		return nullptr;
	auto pagePtr = page(id);
	if(!pagePtr)
		return nullptr;
	return dynamic_cast<BaseWizardPage *>(pagePtr);
}

BaseWizardPage * SetupWizard::getCurrentBasePage()
{
	return getBasePage(currentId());
}

void SetupWizard::pageChanged(int id)
{
	auto basePagePtr = getBasePage(id);
	if(!basePagePtr)
	{
		return;
	}
	if(basePagePtr->wantsRefreshButton())
	{
		setButtonLayout({QWizard::CustomButton1, QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton});
		auto customButton = button(QWizard::CustomButton1);
		connect(customButton, &QAbstractButton::pressed, [&](){
			auto basePagePtr = getCurrentBasePage();
			if(basePagePtr)
			{
				basePagePtr->refresh();
			}
		});
	}
	else
	{
		setButtonLayout({QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton});
	}
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

bool SetupWizard::isRequired()
{
	if (LanguageWizardPage::isRequired())
		return true;
	if (JavaWizardPage::isRequired())
		return true;
	if (AnalyticsWizardPage::isRequired())
		return true;
	return false;
}

