#pragma once

#include <QDialog>
#include "logic/screenshots/ScreenshotList.h"

class ImgurAlbumCreation;

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

	QString message() const;
	QList<ScreenshotPtr> selected() const;

private
slots:
	void on_uploadBtn_clicked();

	void on_deleteBtn_clicked();

private:
	Ui::ScreenshotDialog *ui;
	ScreenshotList *m_list;
	QList<ScreenshotPtr> m_uploaded;
	std::shared_ptr<ImgurAlbumCreation> m_imgurAlbum;
};
