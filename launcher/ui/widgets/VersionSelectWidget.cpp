#include "VersionSelectWidget.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QProgressBar>
#include <QVBoxLayout>

#include "VersionProxyModel.h"

#include "ui/dialogs/CustomMessageBox.h"

VersionSelectWidget::VersionSelectWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("VersionSelectWidget"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    m_proxyModel = new VersionProxyModel(this);

    listView = new VersionListView(this);
    listView->setObjectName(QStringLiteral("listView"));
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setAlternatingRowColors(true);
    listView->setRootIsDecorated(false);
    listView->setItemsExpandable(false);
    listView->setWordWrap(true);
    listView->header()->setCascadingSectionResizes(true);
    listView->header()->setStretchLastSection(false);
    listView->setModel(m_proxyModel);
    verticalLayout->addWidget(listView);

    search = new QLineEdit(this);
    search->setPlaceholderText(tr("Search"));
    search->setClearButtonEnabled(true);
    verticalLayout->addWidget(search);
    connect(search, &QLineEdit::textEdited, [this](const QString& value) {
        m_proxyModel->setSearch(value);
        if (!value.isEmpty() || !listView->selectionModel()->hasSelection()) {
            const QModelIndex first = listView->model()->index(0, 0);
            listView->selectionModel()->setCurrentIndex(first, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            listView->scrollToTop();
        } else
            listView->scrollTo(listView->selectionModel()->currentIndex(), QAbstractItemView::PositionAtCenter);
    });
    search->installEventFilter(this);

    sneakyProgressBar = new QProgressBar(this);
    sneakyProgressBar->setObjectName(QStringLiteral("sneakyProgressBar"));
    sneakyProgressBar->setFormat(QStringLiteral("%p%"));
    verticalLayout->addWidget(sneakyProgressBar);
    sneakyProgressBar->setHidden(true);
    connect(listView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &VersionSelectWidget::currentRowChanged);

    QMetaObject::connectSlotsByName(this);
}

void VersionSelectWidget::setCurrentVersion(const QString& version)
{
    m_currentVersion = version;
    m_proxyModel->setCurrentVersion(version);
}

void VersionSelectWidget::setEmptyString(QString emptyString)
{
    listView->setEmptyString(emptyString);
}

void VersionSelectWidget::setEmptyErrorString(QString emptyErrorString)
{
    listView->setEmptyErrorString(emptyErrorString);
}

void VersionSelectWidget::setEmptyMode(VersionListView::EmptyMode mode)
{
    listView->setEmptyMode(mode);
}

VersionSelectWidget::~VersionSelectWidget() {}

void VersionSelectWidget::setResizeOn(int column)
{
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
    resizeOnColumn = column;
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

bool VersionSelectWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == search && event->type() == QEvent::KeyPress) {
        const QKeyEvent* keyEvent = (QKeyEvent*)event;
        const bool up = keyEvent->key() == Qt::Key_Up;
        const bool down = keyEvent->key() == Qt::Key_Down;
        if (up || down) {
            const QModelIndex index = listView->model()->index(listView->currentIndex().row() + (up ? -1 : 1), 0);
            if (index.row() >= 0 && index.row() < listView->model()->rowCount()) {
                listView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                return true;
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

void VersionSelectWidget::initialize(BaseVersionList* vlist, bool forceLoad)
{
    m_vlist = vlist;
    m_proxyModel->setSourceModel(vlist);
    listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

    if (!m_vlist->isLoaded() || forceLoad) {
        loadList();
    } else {
        if (m_proxyModel->rowCount() == 0) {
            listView->setEmptyMode(VersionListView::String);
        }
        preselect();
    }
}

void VersionSelectWidget::closeEvent(QCloseEvent* event)
{
    QWidget::closeEvent(event);
}

void VersionSelectWidget::loadList()
{
    m_load_task = m_vlist->getLoadTask();
    connect(m_load_task.get(), &Task::succeeded, this, &VersionSelectWidget::onTaskSucceeded);
    connect(m_load_task.get(), &Task::failed, this, &VersionSelectWidget::onTaskFailed);
    connect(m_load_task.get(), &Task::progress, this, &VersionSelectWidget::changeProgress);
    if (!m_load_task->isRunning()) {
        m_load_task->start();
    }
    sneakyProgressBar->setHidden(false);
}

void VersionSelectWidget::onTaskSucceeded()
{
    if (m_proxyModel->rowCount() == 0) {
        listView->setEmptyMode(VersionListView::String);
    }
    sneakyProgressBar->setHidden(true);
    preselect();
    m_load_task.reset();
}

void VersionSelectWidget::onTaskFailed(const QString& reason)
{
    CustomMessageBox::selectable(this, tr("Error"), tr("List update failed:\n%1").arg(reason), QMessageBox::Warning)->show();
    onTaskSucceeded();
}

void VersionSelectWidget::changeProgress(qint64 current, qint64 total)
{
    sneakyProgressBar->setMaximum(total);
    sneakyProgressBar->setValue(current);
}

void VersionSelectWidget::currentRowChanged(const QModelIndex& current, const QModelIndex&)
{
    auto variant = m_proxyModel->data(current, BaseVersionList::VersionPointerRole);
    emit selectedVersionChanged(variant.value<BaseVersion::Ptr>());
}

void VersionSelectWidget::preselect()
{
    if (preselectedAlready)
        return;
    selectCurrent();
    if (preselectedAlready)
        return;
    selectRecommended();
}

void VersionSelectWidget::selectCurrent()
{
    if (m_currentVersion.isEmpty()) {
        return;
    }
    auto idx = m_proxyModel->getVersion(m_currentVersion);
    if (idx.isValid()) {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void VersionSelectWidget::selectSearch()
{
    search->setFocus();
}

VersionListView* VersionSelectWidget::view()
{
    return listView;
}

void VersionSelectWidget::selectRecommended()
{
    auto idx = m_proxyModel->getRecommended();
    if (idx.isValid()) {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

bool VersionSelectWidget::hasVersions() const
{
    return m_proxyModel->rowCount(QModelIndex()) != 0;
}

BaseVersion::Ptr VersionSelectWidget::selectedVersion() const
{
    auto currentIndex = listView->selectionModel()->currentIndex();
    auto variant = m_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
    return variant.value<BaseVersion::Ptr>();
}

void VersionSelectWidget::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_proxyModel->setFilter(role, new ContainsFilter(filter));
}

void VersionSelectWidget::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_proxyModel->setFilter(role, new ExactFilter(filter));
}

void VersionSelectWidget::setExactIfPresentFilter(BaseVersionList::ModelRoles role, QString filter)
{
    m_proxyModel->setFilter(role, new ExactIfPresentFilter(filter));
}

void VersionSelectWidget::setFilter(BaseVersionList::ModelRoles role, Filter* filter)
{
    m_proxyModel->setFilter(role, filter);
}
