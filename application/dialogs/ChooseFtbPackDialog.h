#pragma once

#include <QDialog>
#include <net/NetJob.h>
#include <modplatform/PackHelpers.h>
#include "ui_ChooseFtbPackDialog.h"
#include <modplatform/PackHelpers.h>
#include "FtbListModel.h"

namespace Ui {
	class ChooseFtbPackDialog;
}

class ChooseFtbPackDialog : public QDialog {

	Q_OBJECT

private:
	Ui::ChooseFtbPackDialog *ui;
	FtbModpack selected;
	QString selectedVersion;
	FtbListModel* listModel;
	FtbFilterModel* filterModel;

private slots:
	void onSortingSelectionChanged(QString data);
	void onVersionSelectionItemChanged(QString data);
	void onPackSelectionChanged(QModelIndex first, QModelIndex second);
public:
	ChooseFtbPackDialog(FtbModpackList packs);
	~ChooseFtbPackDialog();

	FtbModpack getSelectedModpack();
	QString getSelectedVersion();
};
