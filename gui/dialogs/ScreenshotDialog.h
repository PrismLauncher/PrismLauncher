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

	QList<ScreenShot *> selected();

private
slots:

private:
	Ui::ScreenshotDialog *ui;
	ScreenshotList *m_list;
};
