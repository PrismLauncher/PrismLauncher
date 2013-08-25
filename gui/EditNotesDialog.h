#pragma once
#include <QDialog>

namespace Ui {
class EditNotesDialog;
}

class EditNotesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditNotesDialog(QString notes, QString name, QWidget *parent = 0);
	~EditNotesDialog();
	QString getText();
private:
	Ui::EditNotesDialog *ui;
	QString m_instance_name;
	QString m_instance_notes;
};
