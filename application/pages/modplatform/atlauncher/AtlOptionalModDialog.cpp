#include "AtlOptionalModDialog.h"
#include "ui_AtlOptionalModDialog.h"

AtlOptionalModListModel::AtlOptionalModListModel(QWidget *parent, QVector<ATLauncher::VersionMod> mods)
    : QAbstractListModel(parent), m_mods(mods) {

    // fill mod index
    for (int i = 0; i < m_mods.size(); i++) {
        auto mod = m_mods.at(i);
        m_index[mod.name] = i;
    }
    // set initial state
    for (int i = 0; i < m_mods.size(); i++) {
        auto mod = m_mods.at(i);
        m_selection[mod.name] = false;
        setMod(mod, i, mod.selected, false);
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

        toggleMod(mod, row);
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

void AtlOptionalModListModel::toggleMod(ATLauncher::VersionMod mod, int index) {
    setMod(mod, index, !m_selection[mod.name]);
}

void AtlOptionalModListModel::setMod(ATLauncher::VersionMod mod, int index, bool enable, bool shouldEmit) {
    if (m_selection[mod.name] == enable) return;

    m_selection[mod.name] = enable;

    // disable other mods in the group, if applicable
    if (enable && !mod.group.isEmpty()) {
        for (int i = 0; i < m_mods.size(); i++) {
            if (index == i) continue;
            auto other = m_mods.at(i);

            if (mod.group == other.group) {
                setMod(other, i, false, shouldEmit);
            }
        }
    }

    for (const auto& dependencyName : mod.depends) {
        auto dependencyIndex = m_index[dependencyName];
        auto dependencyMod = m_mods.at(dependencyIndex);

        // enable/disable dependencies
        if (enable) {
            setMod(dependencyMod, dependencyIndex, true, shouldEmit);
        }

        // if the dependency is 'effectively hidden', then track which mods
        // depend on it - so we can efficiently disable it when no more dependents
        // depend on it.
        auto dependants = m_dependants[dependencyName];

        if (enable) {
            dependants.append(mod.name);
        }
        else {
            dependants.removeAll(mod.name);

            // if there are no longer any dependents, let's disable the mod
            if (dependencyMod.effectively_hidden && dependants.isEmpty()) {
                setMod(dependencyMod, dependencyIndex, false, shouldEmit);
            }
        }
    }

    // disable mods that depend on this one, if disabling
    if (!enable) {
        auto dependants = m_dependants[mod.name];
        for (const auto& dependencyName : dependants) {
            auto dependencyIndex = m_index[dependencyName];
            auto dependencyMod = m_mods.at(dependencyIndex);

            setMod(dependencyMod, dependencyIndex, false, shouldEmit);
        }
    }

    if (shouldEmit) {
        emit dataChanged(AtlOptionalModListModel::index(index, EnabledColumn),
                         AtlOptionalModListModel::index(index, EnabledColumn));
    }
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
