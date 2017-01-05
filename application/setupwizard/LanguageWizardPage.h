#pragma once

#include "BaseWizardPage.h"

class QVBoxLayout;
class QListView;

class LanguageWizardPage : public BaseWizardPage
{
	Q_OBJECT;
public:
	explicit LanguageWizardPage(QWidget *parent = Q_NULLPTR);

	virtual ~LanguageWizardPage();

	bool wantsRefreshButton() override;

	void refresh() override;

	bool validatePage() override;

	static bool isRequired();

protected:
	void retranslate() override;

protected slots:
	void languageRowChanged(const QModelIndex &current, const QModelIndex &previous);

private:
	QVBoxLayout *verticalLayout = nullptr;
	QListView *languageView = nullptr;
};
