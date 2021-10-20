#include "LanguageSelectionWidget.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>
#include "Launcher.h"
#include "translations/TranslationsModel.h"

LanguageSelectionWidget::LanguageSelectionWidget(QWidget *parent) :
    QWidget(parent)
{
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
    helpUsLabel->setObjectName(QStringLiteral("helpUsLabel"));
    helpUsLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    helpUsLabel->setOpenExternalLinks(true);
    helpUsLabel->setWordWrap(true);
    verticalLayout->addWidget(helpUsLabel);

    auto translations = LAUNCHER->translations();
    auto index = translations->selectedIndex();
    languageView->setModel(translations.get());
    languageView->setCurrentIndex(index);
    languageView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    languageView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(languageView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LanguageSelectionWidget::languageRowChanged);
    verticalLayout->setContentsMargins(0,0,0,0);
}

QString LanguageSelectionWidget::getSelectedLanguageKey() const
{
    auto translations = LAUNCHER->translations();
    return translations->data(languageView->currentIndex(), Qt::UserRole).toString();
}

void LanguageSelectionWidget::retranslate()
{
    QString text = tr("Don't see your language or the quality is poor?<br/><a href=\"%1\">Help us with translations!</a>")
        .arg("https://github.com/MultiMC/Launcher/wiki/Translating-MultiMC");
    helpUsLabel->setText(text);

}

void LanguageSelectionWidget::languageRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    if (current == previous)
    {
        return;
    }
    auto translations = LAUNCHER->translations();
    QString key = translations->data(current, Qt::UserRole).toString();
    translations->selectLanguage(key);
    translations->updateLanguage(key);
}
