#include "AtlOptionalModDialog.h"
#include "ui_AtlOptionalModDialog.h"

AtlOptionalModListModel::AtlOptionalModListModel(QWidget *parent, QVector<ATLauncher::VersionMod> mods)
    : QAbstractListModel(parent), m_mods(mods) {

    for (const auto& mod : mods) {
        m_selection[mod.name] = mod.selected;
    }
}

QVector<QString> AtlOptionalModListModel::getResult() {
    QVector<QString> result;

    for (const auto& mod : m_mods) {
        if (m_selection[mod.name]) {
            result.push_back(mod.name);
        }
    }

    return result;
}

int AtlOptionalModListModel::rowCount(const QModelIndex &parent) const {
    return m_mods.size();
}

int AtlOptionalModListModel::columnCount(const QModelIndex &parent) const {
    // Enabled, Name, Description
    return 3;
}

QVariant AtlOptionalModListModel::data(const QModelIndex &index, int role) const {
    auto row = index.row();
    auto mod = m_mods.at(row);

    if (role == Qt::DisplayRole) {
        if (index.column() == NameColumn) {
            return mod.name;
        }
        if (index.column() == DescriptionColumn) {
            return mod.description;
        }
    }
    else if (role == Qt::ToolTipRole) {
        if (index.column() == DescriptionColumn) {
            return mod.description;
        }
    }
    else if (role == Qt::CheckStateRole) {
        if (index.column() == EnabledColumn) {
            return m_selection[mod.name] ? Qt::Checked : Qt::Unchecked;
        }
    }

    return QVariant();
}

bool AtlOptionalModListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::CheckStateRole) {
        auto row = index.row();
        auto mod = m_mods.at(row);

        // toggle the state
        m_selection[mod.name] = !m_selection[mod.name];

        emit dataChanged(AtlOptionalModListModel::index(index.row(), EnabledColumn),
                         AtlOptionalModListModel::index(index.row(), EnabledColumn));
        return true;
    }

    return false;
}

QVariant AtlOptionalModListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case EnabledColumn:
                return QString();
            case NameColumn:
                return QString("Name");
            case DescriptionColumn:
                return QString("Description");
        }
    }

    return QVariant();
}

Qt::ItemFlags AtlOptionalModListModel::flags(const QModelIndex &index) const {
    auto flags = QAbstractListModel::flags(index);
    if (index.isValid() && index.column() == EnabledColumn) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

void AtlOptionalModListModel::selectRecommended() {
    for (const auto& mod : m_mods) {
        m_selection[mod.name] = mod.recommended;
    }

    emit dataChanged(AtlOptionalModListModel::index(0, EnabledColumn),
                     AtlOptionalModListModel::index(m_mods.size() - 1, EnabledColumn));
}

void AtlOptionalModListModel::clearAll() {
    for (const auto& mod : m_mods) {
        m_selection[mod.name] = false;
    }

    emit dataChanged(AtlOptionalModListModel::index(0, EnabledColumn),
                     AtlOptionalModListModel::index(m_mods.size() - 1, EnabledColumn));
}


AtlOptionalModDialog::AtlOptionalModDialog(QWidget *parent, QVector<ATLauncher::VersionMod> mods)
    : QDialog(parent), ui(new Ui::AtlOptionalModDialog) {
    ui->setupUi(this);

    listModel = new AtlOptionalModListModel(this, mods);
    ui->treeView->setModel(listModel);

    ui->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->treeView->header()->setSectionResizeMode(
            AtlOptionalModListModel::NameColumn, QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(
            AtlOptionalModListModel::DescriptionColumn, QHeaderView::Stretch);

    connect(ui->selectRecommendedButton, &QPushButton::pressed,
            listModel, &AtlOptionalModListModel::selectRecommended);
    connect(ui->clearAllButton, &QPushButton::pressed,
            listModel, &AtlOptionalModListModel::clearAll);
}

AtlOptionalModDialog::~AtlOptionalModDialog() {
    delete ui;
}
