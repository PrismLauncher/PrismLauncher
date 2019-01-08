#pragma once

#include "BaseWizardPage.h"

class QVBoxLayout;
class QTreeView;
class QLabel;

class LanguageWizardPage : public BaseWizardPage
{
    Q_OBJECT
public:
    explicit LanguageWizardPage(QWidget *parent = Q_NULLPTR);

    virtual ~LanguageWizardPage();

    bool wantsRefreshButton() override;

    void refresh() override;

    bool validatePage() override;

protected:
    void retranslate() override;

protected slots:
    void languageRowChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    QVBoxLayout *verticalLayout = nullptr;
    QTreeView *languageView = nullptr;
    QLabel *helpUsLabel = nullptr;
};
