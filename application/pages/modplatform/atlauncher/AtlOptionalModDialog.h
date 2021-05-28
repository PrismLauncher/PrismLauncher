#pragma once

#include <QDialog>
#include <QAbstractListModel>

#include "modplatform/atlauncher/ATLPackIndex.h"

namespace Ui {
class AtlOptionalModDialog;
}

class AtlOptionalModListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Columns
    {
        EnabledColumn = 0,
        NameColumn,
        DescriptionColumn,
    };

    AtlOptionalModListModel(QWidget *parent, QVector<ATLauncher::VersionMod> mods);

    QVector<QString> getResult();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void selectRecommended();
    void clearAll();

private:
    QVector<ATLauncher::VersionMod> m_mods;
    QMap<QString, bool> m_selection;
};

class AtlOptionalModDialog : public QDialog {
    Q_OBJECT

public:
    AtlOptionalModDialog(QWidget *parent, QVector<ATLauncher::VersionMod> mods);
    ~AtlOptionalModDialog() override;

    QVector<QString> getResult() {
        return listModel->getResult();
    }

private:
    Ui::AtlOptionalModDialog *ui;

    AtlOptionalModListModel *listModel;
};
