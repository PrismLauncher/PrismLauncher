//
// Created by Kenneth Chew on 11/11/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MacSandboxPage.h" resolved

#include "MacSandboxPage.h"

#include <QFileDialog>
#include <QListWidgetItem>

#include "Application.h"
#include "FastFileIconProvider.h"
#include "macsandbox/DynamicSandboxException.h"
#include "ui/widgets/DropList.h"
#include "ui_MacSandboxPage.h"

MacSandboxPage::MacSandboxPage(QWidget* parent) : QMainWindow(parent), ui(new Ui::MacSandboxPage)
{
    ui->setupUi(this);

    connect(ui->readWriteList, &DropList::droppedURLs, APPLICATION->m_dynamicSandboxExceptions.get(), &DynamicSandboxException::addReadWriteExceptions);
    connect(ui->readOnlyList, &DropList::droppedURLs, APPLICATION->m_dynamicSandboxExceptions.get(), &DynamicSandboxException::addReadOnlyExceptions);
    connect(ui->readWriteList, &DropList::droppedURLs, this, &MacSandboxPage::loadSettings);
    connect(ui->readOnlyList, &DropList::droppedURLs, this, &MacSandboxPage::loadSettings);

    connect(ui->readWriteList, &DropList::deleteKeyPressed, this, &MacSandboxPage::on_readWriteRemoveBtn_clicked);
    connect(ui->readOnlyList, &DropList::deleteKeyPressed, this, &MacSandboxPage::on_readOnlyRemoveBtn_clicked);
}

MacSandboxPage::~MacSandboxPage()
{
    delete ui;
}

void MacSandboxPage::loadSettings()
{
    auto s = APPLICATION->settings();

    // macOS sandbox user-selected dynamic exceptions
    QList<QUrl> readWriteURLs = APPLICATION->m_dynamicSandboxExceptions->readWriteExceptionURLs();
    QList<QUrl> readOnlyURLs = APPLICATION->m_dynamicSandboxExceptions->readOnlyExceptionURLs();

    QFileIconProvider iconProvider;
    ui->readWriteList->clear();
    for (const QUrl& url : readWriteURLs) {
        if (url.isEmpty())
            continue;
        if (url.scheme() == "file") {
            QIcon fileIcon = iconProvider.icon(QFileInfo(url.toLocalFile()));
            auto item = new QListWidgetItem(fileIcon, url.toLocalFile());
            ui->readWriteList->addItem(item);
        }
    }
    ui->readOnlyList->clear();
    for (const QUrl& url : readOnlyURLs) {
        if (url.isEmpty())
            continue;
        if (url.scheme() == "file") {
            QIcon fileIcon = iconProvider.icon(QFileInfo(url.toLocalFile()));
            auto item = new QListWidgetItem(fileIcon, url.toLocalFile());
            ui->readOnlyList->addItem(item);
        }
    }
}

void MacSandboxPage::on_readWriteAddBtn_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Add Read/Write Exception"), QDir::homePath());
    if (!dir.isEmpty()) {
        APPLICATION->m_dynamicSandboxExceptions->addReadWriteException(dir);
        loadSettings();
    }
}

void MacSandboxPage::on_readWriteRemoveBtn_clicked() {
    int row = ui->readWriteList->currentRow();
    if (row >= 0) {
        APPLICATION->m_dynamicSandboxExceptions->removeReadWriteException(row);
        loadSettings();
    }
}

void MacSandboxPage::on_readOnlyAddBtn_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Add Read Only Exception"), QDir::homePath());
    if (!dir.isEmpty()) {
        APPLICATION->m_dynamicSandboxExceptions->addReadOnlyException(dir);
        loadSettings();
    }
}

void MacSandboxPage::on_readOnlyRemoveBtn_clicked() {
    int row = ui->readOnlyList->currentRow();
    if (row >= 0) {
        APPLICATION->m_dynamicSandboxExceptions->removeReadOnlyException(row);
        loadSettings();
    }
}
