#pragma once

#include <QDialog>
#include "logic/lists/ScreenshotList.h"

class BaseInstance;

namespace Ui
{
class ScreenshotDialog;
}

class ScreenshotDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ScreenshotDialog(ScreenshotList *list, QWidget *parent = 0);
	~ScreenshotDialog();

	enum
	{
		NothingDone = 0x42
	};

	QList<ScreenShot *> uploaded() const;

private
slots:
	void on_buttonBox_accepted();

private:
	Ui::ScreenshotDialog *ui;
	ScreenshotList *m_list;
	QList<ScreenShot *> m_uploaded;

	QList<ScreenShot *> selected() const;
};
