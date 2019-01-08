#include "LanguageWizardPage.h"
#include <MultiMC.h>
#include <translations/TranslationsModel.h>

#include <QVBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>

LanguageWizardPage::LanguageWizardPage(QWidget *parent)
    : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("languagePage"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    languageView = new QTreeView(this);
    languageView->setObjectName(QStringLiteral("languageView"));
    languageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    languageView->setAlternatingRowColors(true);
    languageView->setRootIsDecorated(false);
    languageView->setItemsExpandable(false);
    languageView->setWordWrap(true);
    languageView->header()->setCascadingSectionResizes(true);
    languageView->header()->setStretchLastSection(false);
    verticalLayout->addWidget(languageView);
    helpUsLabel = new QLabel(this);
    helpUsLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    helpUsLabel->setOpenExternalLinks(true);
    helpUsLabel->setWordWrap(true);
    verticalLayout->addWidget(helpUsLabel);
    retranslate();

    auto translations = MMC->translations();
    auto index = translations->selectedIndex();
    languageView->setModel(translations.get());
    languageView->setCurrentIndex(index);
    languageView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    languageView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
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

void LanguageWizardPage::retranslate()
{
    setTitle(tr("Language"));
    setSubTitle(tr("Select the language to use in MultiMC"));
    QString text =
        tr("Don't see your language or the quality is poor?") +
        "<br/>" +
        QString("<a href=\"https://github.com/MultiMC/MultiMC5/wiki/Translating-MultiMC\">%1</a>").arg("Help us with translations!");
    helpUsLabel->setText(text);
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
