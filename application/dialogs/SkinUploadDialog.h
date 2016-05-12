#pragma once

#include <QDialog>
#include <minecraft/auth/MojangAccount.h>

namespace Ui
{
	class SkinUploadDialog;
}

class SkinUploadDialog : public QDialog {
	Q_OBJECT
public:
	explicit SkinUploadDialog(MojangAccountPtr acct, QWidget *parent = 0);
	virtual ~SkinUploadDialog() {};

public slots:
	void on_buttonBox_accepted();

	void on_buttonBox_rejected();

	void on_skinBrowseBtn_clicked();

protected:
	MojangAccountPtr m_acct;

private:
	Ui::SkinUploadDialog *ui;
};
