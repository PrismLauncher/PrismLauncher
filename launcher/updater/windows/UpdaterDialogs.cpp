#include "UpdaterDialogs.h"

#include "ui_SelectReleaseDialog.h"

#include <QTextBrowser>
#include "Markdown.h"

SelectReleaseDialog::SelectReleaseDialog(const Version& current_version, const QList<GitHubRelease>& releases, QWidget* parent)
    : QDialog(parent), m_releases(releases), m_currentVersion(current_version), ui(new Ui::SelectReleaseDialog)
{
    ui->setupUi(this);
    
    ui->changelogTextBrowser->setOpenExternalLinks(true);
    ui->changelogTextBrowser->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    ui->changelogTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    
    ui->versionsTree->setColumnCount(2);

    ui->versionsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->versionsTree->header()->setSectionResizeMode(1, QHeaderView::Stretch); 
    ui->versionsTree->setHeaderLabels({tr("Verison"), tr("Published Date")});
    ui->versionsTree->header()->setStretchLastSection(false);
    
    ui->eplainLabel->setText(tr("Select a version to install.\n"
                                 "\n"
                                 "Currently installed version: %1")
                                  .arg(m_currentVersion.toString()));
    
    loadReleases();
    
    connect(ui->versionsTree, &QTreeWidget::currentItemChanged, this, &SelectReleaseDialog::selectionChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SelectReleaseDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SelectReleaseDialog::reject);
}

SelectReleaseDialog::~SelectReleaseDialog()
{
    delete ui;
}

void SelectReleaseDialog::loadReleases()
{
    for (auto rls : m_releases) {
        appendRelease(rls);
    }
}

void SelectReleaseDialog::appendRelease(GitHubRelease const& release)
{
    auto rls_item = new QTreeWidgetItem(ui->versionsTree);
    rls_item->setText(0, release.tag_name);
    rls_item->setExpanded(true);
    rls_item->setText(1, release.published_at.toString());
    rls_item->setData(0, Qt::UserRole, QVariant(release.id));

    ui->versionsTree->addTopLevelItem(rls_item);
}

GitHubRelease SelectReleaseDialog::getRelease(QTreeWidgetItem* item) {
    int id = item->data(0, Qt::UserRole).toInt();
    GitHubRelease release;
    for (auto rls: m_releases) {
        if (rls.id == id)
            release = rls;
    }
    return release;
}

void SelectReleaseDialog::selectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    GitHubRelease release = getRelease(current);
    QString body = markdownToHTML(release.body.toUtf8());
    m_selectedRelease = release;

    ui->changelogTextBrowser->setHtml(body);
}

