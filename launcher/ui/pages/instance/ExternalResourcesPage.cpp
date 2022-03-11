#include "ExternalResourcesPage.h"
#include "ui_ExternalResourcesPage.h"

#include "DesktopServices.h"
#include "Version.h"
#include "minecraft/mod/ModFolderModel.h"
#include "ui/GuiUtil.h"

#include <QKeyEvent>
#include <QMenu>

namespace {
// FIXME: wasteful
void RemoveThePrefix(QString& string)
{
    QRegularExpression regex(QStringLiteral("^(([Tt][Hh][eE])|([Tt][eE][Hh])) +"));
    string.remove(regex);
    string = string.trimmed();
}
}  // namespace

class SortProxy : public QSortFilterProxyModel {
   public:
    explicit SortProxy(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        ModFolderModel* model = qobject_cast<ModFolderModel*>(sourceModel());
        if (!model)
            return false;
        
        const auto& mod = model->at(source_row);

        if (mod.name().contains(filterRegExp()))
            return true;
        if (mod.description().contains(filterRegExp()))
            return true;
        
        for (auto& author : mod.authors()) {
            if (author.contains(filterRegExp())) {
                return true;
            }
        }

        return false;
    }

    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override
    {
        ModFolderModel* model = qobject_cast<ModFolderModel*>(sourceModel());
        if (!model || !source_left.isValid() || !source_right.isValid() || source_left.column() != source_right.column()) {
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        // we are now guaranteed to have two valid indexes in the same column... we love the provided invariants unconditionally and
        // proceed.

        auto column = (ModFolderModel::Columns) source_left.column();
        bool invert = false;
        switch (column) {
            // GH-2550 - sort by enabled/disabled
            case ModFolderModel::ActiveColumn: {
                auto dataL = source_left.data(Qt::CheckStateRole).toBool();
                auto dataR = source_right.data(Qt::CheckStateRole).toBool();
                if (dataL != dataR)
                    return dataL > dataR;
                
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2722 - sort mod names in a way that discards "The" prefixes
            case ModFolderModel::NameColumn: {
                auto dataL = model->data(model->index(source_left.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataL);
                auto dataR = model->data(model->index(source_right.row(), ModFolderModel::NameColumn)).toString();
                RemoveThePrefix(dataR);

                auto less = dataL.compare(dataR, sortCaseSensitivity());
                if (less != 0)
                    return invert ? (less > 0) : (less < 0);
                
                // fallthrough
                invert = sortOrder() == Qt::DescendingOrder;
            }
            // GH-2762 - sort versions by parsing them as versions
            case ModFolderModel::VersionColumn: {
                auto dataL = Version(model->data(model->index(source_left.row(), ModFolderModel::VersionColumn)).toString());
                auto dataR = Version(model->data(model->index(source_right.row(), ModFolderModel::VersionColumn)).toString());
                return invert ? (dataL > dataR) : (dataL < dataR);
            }
            default: {
                return QSortFilterProxyModel::lessThan(source_left, source_right);
            }
        }
    }
};

ExternalResourcesPage::ExternalResourcesPage(BaseInstance* instance, std::shared_ptr<ModFolderModel> model, QWidget* parent)
    : QMainWindow(parent), m_instance(instance), ui(new Ui::ExternalResourcesPage), m_model(model)
{
    ui->setupUi(this);

    runningStateChanged(m_instance && m_instance->isRunning());

    ui->actionsToolbar->insertSpacer(ui->actionViewConfigs);

    m_filterModel = new SortProxy(this);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_model.get());
    m_filterModel->setFilterKeyColumn(-1);
    ui->treeView->setModel(m_filterModel);

    ui->treeView->installEventFilter(this);
    ui->treeView->sortByColumn(1, Qt::AscendingOrder);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    // The default function names by Qt are pretty ugly, so let's just connect the actions manually,
    // to make it easier to read :)
    connect(ui->actionAddItem, &QAction::triggered, this, &ExternalResourcesPage::addItem);
    connect(ui->actionRemoveItem, &QAction::triggered, this, &ExternalResourcesPage::removeItem);
    connect(ui->actionEnableItem, &QAction::triggered, this, &ExternalResourcesPage::enableItem);
    connect(ui->actionDisableItem, &QAction::triggered, this, &ExternalResourcesPage::disableItem);
    connect(ui->actionViewConfigs, &QAction::triggered, this, &ExternalResourcesPage::viewConfigs);
    connect(ui->actionViewFolder, &QAction::triggered, this, &ExternalResourcesPage::viewFolder);

    connect(ui->treeView, &ModListView::customContextMenuRequested, this, &ExternalResourcesPage::ShowContextMenu);
    connect(ui->treeView, &ModListView::activated, this, &ExternalResourcesPage::itemActivated);

    auto selection_model = ui->treeView->selectionModel();
    connect(selection_model, &QItemSelectionModel::currentChanged, this, &ExternalResourcesPage::current);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ExternalResourcesPage::filterTextChanged);
    connect(m_instance, &BaseInstance::runningStatusChanged, this, &ExternalResourcesPage::runningStateChanged);
}

ExternalResourcesPage::~ExternalResourcesPage()
{
    m_model->stopWatching();
    delete ui;
}

void ExternalResourcesPage::itemActivated(const QModelIndex&)
{
    if (!m_controlsEnabled)
        return;
    
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    m_model->setModStatus(selection.indexes(), ModFolderModel::Toggle);
}

QMenu* ExternalResourcesPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->actionsToolbar->toggleViewAction());
    return filteredMenu;
}

void ExternalResourcesPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->treeView->mapToGlobal(pos));
    delete menu;
}

void ExternalResourcesPage::openedImpl()
{
    m_model->startWatching();
}

void ExternalResourcesPage::closedImpl()
{
    m_model->stopWatching();
}

void ExternalResourcesPage::retranslate()
{
    ui->retranslateUi(this);
}

void ExternalResourcesPage::filterTextChanged(const QString& newContents)
{
    m_viewFilter = newContents;
    m_filterModel->setFilterFixedString(m_viewFilter);
}

void ExternalResourcesPage::runningStateChanged(bool running)
{
    if (m_controlsEnabled == !running)
        return;
    
    m_controlsEnabled = !running;
    ui->actionAddItem->setEnabled(m_controlsEnabled);
    ui->actionDisableItem->setEnabled(m_controlsEnabled);
    ui->actionEnableItem->setEnabled(m_controlsEnabled);
    ui->actionRemoveItem->setEnabled(m_controlsEnabled);
}

bool ExternalResourcesPage::shouldDisplay() const
{
    return true;
}

bool ExternalResourcesPage::listFilter(QKeyEvent* keyEvent)
{
    switch (keyEvent->key()) {
        case Qt::Key_Delete:
            removeItem();
            return true;
        case Qt::Key_Plus:
            addItem();
            return true;
        default:
            break;
    }
    return QWidget::eventFilter(ui->treeView, keyEvent);
}

bool ExternalResourcesPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, ev);
    
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
    if (obj == ui->treeView)
        return listFilter(keyEvent);

    return QWidget::eventFilter(obj, ev);
}

void ExternalResourcesPage::addItem()
{
    if (!m_controlsEnabled)
        return;
    

    auto list = GuiUtil::BrowseForFiles(
        helpPage(), tr("Select %1", "Select whatever type of files the page contains. Example: 'Loader Mods'").arg(displayName()),
        m_fileSelectionFilter.arg(displayName()), APPLICATION->settings()->get("CentralModsDir").toString(), this->parentWidget());

    if (!list.isEmpty()) {
        for (auto filename : list) {
            m_model->installMod(filename);
        }
    }
}

void ExternalResourcesPage::removeItem()
{
    if (!m_controlsEnabled)
        return;
    
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    m_model->deleteMods(selection.indexes());
}

void ExternalResourcesPage::enableItem()
{
    if (!m_controlsEnabled)
        return;
    
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    m_model->setModStatus(selection.indexes(), ModFolderModel::Enable);
}

void ExternalResourcesPage::disableItem()
{
    if (!m_controlsEnabled)
        return;
    
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    m_model->setModStatus(selection.indexes(), ModFolderModel::Disable);
}

void ExternalResourcesPage::viewConfigs()
{
    DesktopServices::openDirectory(m_instance->instanceConfigFolder(), true);
}

void ExternalResourcesPage::viewFolder()
{
    DesktopServices::openDirectory(m_model->dir().absolutePath(), true);
}

void ExternalResourcesPage::current(const QModelIndex& current, const QModelIndex& previous)
{
    if (!current.isValid()) {
        ui->frame->clear();
        return;
    }

    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    Mod& m = m_model->operator[](row);
    ui->frame->updateWithMod(m);
}
