#pragma once

#include "BaseWizardPage.h"

class QVBoxLayout;
class QTextBrowser;
class QCheckBox;

class AnalyticsWizardPage : public BaseWizardPage
{
	Q_OBJECT;
public:
	explicit AnalyticsWizardPage(QWidget *parent = Q_NULLPTR);
	virtual ~AnalyticsWizardPage();

	bool validatePage() override;
	static bool isRequired();

protected:
	void retranslate() override;

private:
	QVBoxLayout *verticalLayout_3 = nullptr;
	QTextBrowser *textBrowser = nullptr;
	QCheckBox *checkBox = nullptr;
};