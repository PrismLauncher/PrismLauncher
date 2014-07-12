#pragma once

#include <QWidget>

#include "BasePage.h"

namespace Ui {
class OtherLogsPage;
}

class RecursiveFileSystemWatcher;

class BaseInstance;

class OtherLogsPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit OtherLogsPage(BaseInstance *instance, QWidget *parent = 0);
	~OtherLogsPage();

	QString id() const override { return "logs"; }
	QString displayName() const override { return tr("Other logs"); }
	QIcon icon() const override  { return QIcon(); } // TODO
	QString helpPage() const override { return "Minecraft-Logs"; }
	void opened() override;
	void closed() override;

private
slots:
	void on_selectLogBox_currentIndexChanged(const int index);
	void on_btnReload_clicked();
	void on_btnPaste_clicked();
	void on_btnCopy_clicked();
	void on_btnDelete_clicked();

private:
	Ui::OtherLogsPage *ui;
	BaseInstance *m_instance;
	RecursiveFileSystemWatcher *m_watcher;
	QString m_currentFile;

	void setControlsEnabled(const bool enabled);
};
