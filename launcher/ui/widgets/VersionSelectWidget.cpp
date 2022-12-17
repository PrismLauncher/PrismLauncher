#include "VersionSelectWidget.h"

#include <QProgressBar>
#include <QVBoxLayout>
#include <QHeaderView>

#include "VersionProxyModel.h"

#include "ui/dialogs/CustomMessageBox.h"

VersionSelectWidget::VersionSelectWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("VersionSelectWidget"));
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel = new VersionProxyModel(this);

    listView = new VersionListView(this);
    listView->setObjectName(QStringLiteral("listView"));
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setAlternatingRowColors(true);
    listView->setRootIsDecorated(false);
    listView->setItemsExpandable(false);
    listView->setWordWrap(true);
    listView->header()->setCascadingSectionResizes(true);
    listView->header()->setStretchLastSection(false);
    listView->setModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel);
    verticalLayout->addWidget(listView);

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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersion = version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setCurrentVersion(version);
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

VersionSelectWidget::~VersionSelectWidget()
{
}

void VersionSelectWidget::setResizeOn(int column)
{
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
    resizeOnColumn = column;
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

void VersionSelectWidget::initialize(BaseVersionList *vlist)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vlist = vlist;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setSourceModel(vlist);
    listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vlist->isLoaded())
    {
        loadList();
    }
    else
    {
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->rowCount() == 0)
        {
            listView->setEmptyMode(VersionListView::String);
        }
        preselect();
    }
}

void VersionSelectWidget::closeEvent(QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

void VersionSelectWidget::loadList()
{
    auto newTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vlist->getLoadTask();
    if (!newTask)
    {
        return;
    }
    loadTask = newTask.get();
    connect(loadTask, &Task::succeeded, this, &VersionSelectWidget::onTaskSucceeded);
    connect(loadTask, &Task::failed, this, &VersionSelectWidget::onTaskFailed);
    connect(loadTask, &Task::progress, this, &VersionSelectWidget::changeProgress);
    if(!loadTask->isRunning())
    {
        loadTask->start();
    }
    sneakyProgressBar->setHidden(false);
}

void VersionSelectWidget::onTaskSucceeded()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->rowCount() == 0)
    {
        listView->setEmptyMode(VersionListView::String);
    }
    sneakyProgressBar->setHidden(true);
    preselect();
    loadTask = nullptr;
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
    auto variant = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->data(current, BaseVersionList::VersionPointerRole);
    emit selectedVersionChanged(variant.value<BaseVersion::Ptr>());
}

void VersionSelectWidget::preselect()
{
    if(preselectedAlready)
        return;
    selectCurrent();
    if(preselectedAlready)
        return;
    selectRecommended();
}

void VersionSelectWidget::selectCurrent()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersion.isEmpty())
    {
        return;
    }
    auto idx = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->getVersion(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersion);
    if(idx.isValid())
    {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void VersionSelectWidget::selectRecommended()
{
    auto idx = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->getRecommended();
    if(idx.isValid())
    {
        preselectedAlready = true;
        listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

bool VersionSelectWidget::hasVersions() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->rowCount(QModelIndex()) != 0;
}

BaseVersion::Ptr VersionSelectWidget::selectedVersion() const
{
    auto currentIndex = listView->selectionModel()->currentIndex();
    auto variant = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
    return variant.value<BaseVersion::Ptr>();
}

void VersionSelectWidget::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setFilter(role, new ExactFilter(filter));
}

void VersionSelectWidget::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setFilter(role, new ContainsFilter(filter));
}

void VersionSelectWidget::setFilter(BaseVersionList::ModelRoles role, Filter *filter)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setFilter(role, filter);
}
