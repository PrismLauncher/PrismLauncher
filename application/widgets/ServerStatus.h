#pragma once
#include <QString>
#include <QWidget>
#include <QMap>
#include <QIcon>
#include <memory>

class IconLabel;
class QToolButton;
class QHBoxLayout;
class StatusChecker;

class ServerStatus: public QWidget
{
	Q_OBJECT
public:
	explicit ServerStatus(QWidget *parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~ServerStatus();

public slots:
	void reloadStatus();
	void StatusChanged(const QMap<QString, QString> statuses);
	void StatusReloading(bool is_reloading);

private slots:
	void clicked();

private: /* methods */
	void addLine();
	void addStatus(QString key, QString name);
	void setStatus(QString key, int value);
private: /* data */
	QHBoxLayout * layout = nullptr;
	QToolButton *m_statusRefresh = nullptr;
	QMap<QString, IconLabel *> serverLabels;
	QIcon goodIcon;
	QIcon yellowIcon;
	QIcon badIcon;
	std::shared_ptr<StatusChecker> m_statusChecker;
};
