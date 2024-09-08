#pragma once

#include <QEvent>
#include <QWizardPage>

class BaseWizardPage : public QWizardPage {
   public:
    explicit BaseWizardPage(QWidget* parent = Q_NULLPTR) : QWizardPage(parent) {}
    virtual ~BaseWizardPage() {};

    virtual bool wantsRefreshButton() { return false; }
    virtual void refresh() {}

   protected:
    virtual void retranslate() = 0;
    void changeEvent(QEvent* event) override
    {
        if (event->type() == QEvent::LanguageChange) {
            retranslate();
        }
        QWizardPage::changeEvent(event);
    }
};
