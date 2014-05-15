#pragma once
#include <QWidget>
#include <memory>

class QToolButton;
class QHBoxLayout;

class ServerStatus: public QWidget
{
	Q_OBJECT
public:
	explicit ServerStatus(QWidget *parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~ServerStatus() {};
public slots:
	void updateStatusUI();

	void updateStatusFailedUI();

	void reloadStatus();
	void StatusChanged();

private: /* methods */
	clear();
	addLine();
	addStatus(QString name, bool online);
private: /* data */
	QHBoxLayout * layout = nullptr;
	QToolButton *m_statusRefresh = nullptr;
	QPixmap goodIcon;
	QPixmap badIcon;
	QTimer statusTimer;
};
