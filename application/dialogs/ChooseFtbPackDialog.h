#pragma once

#include <QDialog>
#include <net/NetJob.h>
#include <modplatform/PackHelpers.h>
#include "ui_ChooseFtbPackDialog.h"
#include <modplatform/PackHelpers.h>

namespace Ui {
	class ChooseFtbPackDialog;
}

class ChooseFtbPackDialog : public QDialog {

	Q_OBJECT

private:
	Ui::ChooseFtbPackDialog *ui;
	FtbModpack selected;
	QString selectedVersion;

private slots:
	void onListItemClicked(QListWidgetItem *item);
	void onVersionSelectionItemChanged(QString data);

public:
	ChooseFtbPackDialog(FtbModpackList packs);
	~ChooseFtbPackDialog();

	FtbModpack getSelectedModpack();
	QString getSelectedVersion();
};
