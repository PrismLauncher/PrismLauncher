#include "ServerStatus.h"
#include "logic/status/StatusChecker.h"

#include "MultiMC.h"

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QMap>
#include <QToolButton>

ServerStatus::ServerStatus(QWidget *parent, Qt::WindowFlags f)
	:QWidget(parent, f)
{
	clear();
	goodIcon = QPixmap(":/icons/multimc/48x48/status-good.png");
	goodIcon.setDevicePixelRatio(2.0);
	badIcon = QPixmap(":/icons/multimc/48x48/status-bad.png");
	badIcon.setDevicePixelRatio(2.0);
	addStatus(tr("No status available"), false);
	m_statusRefresh = new QToolButton(this);
	m_statusRefresh->setCheckable(true);
	m_statusRefresh->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_statusRefresh->setIcon(QIcon::fromTheme("refresh"));
	// Start status checker
	{
		connect(MMC->statusChecker().get(), &StatusChecker::statusLoaded, this,
				&ServerStatus::updateStatusUI);
		connect(MMC->statusChecker().get(), &StatusChecker::statusLoadingFailed, this,
				&ServerStatus::updateStatusFailedUI);

		connect(m_statusRefresh, &QAbstractButton::clicked, this, &ServerStatus::reloadStatus);
		connect(&statusTimer, &QTimer::timeout, this, &ServerStatus::reloadStatus);
		statusTimer.setSingleShot(true);

		reloadStatus();
	}

}

ServerStatus::addLine()
{
	auto line = new QFrame(this);
	line->setFrameShape(QFrame::VLine);
	line->setFrameShadow(QFrame::Sunken);
	layout->addWidget(line);
}

ServerStatus::addStatus(QString name, bool online)
{
	auto label = new QLabel(this);
	label->setText(name);
	if(online)
		label->setPixmap(goodIcon);
	else
		label->setPixmap(badIcon);
	layout->addWidget(label);
}

ServerStatus::clear()
{
	if(layout)
		delete layout;
	layout = new QHBoxLayout(this);
}

void ServerStatus::StatusChanged(QMap<QString, QString> statusEntries)
{
	clear();
	int howmany = statusEntries.size();
	int index = 0;
	auto iter = statusEntries.begin();
	while (iter != statusEntries.end())
	{
		addStatus();
		index++;
	}
}

static QString convertStatus(const QString &status)
{
	QString ret = "?";

	if (status == "green")
		ret = "↑";
	else if (status == "yellow")
		ret = "-";
	else if (status == "red")
		ret = "↓";

	return "<span style=\"font-size:11pt; font-weight:600;\">" + ret + "</span>";
}

void ServerStatus::reloadStatus()
{
	m_statusRefresh->setChecked(true);
	MMC->statusChecker()->reloadStatus();
	// updateStatusUI();
}

static QString makeStatusString(const QMap<QString, QString> statuses)
{
	QString status = "";
	status += "Web: " + convertStatus(statuses["minecraft.net"]);
	status += "  Account: " + convertStatus(statuses["account.mojang.com"]);
	status += "  Skins: " + convertStatus(statuses["skins.minecraft.net"]);
	status += "  Auth: " + convertStatus(statuses["authserver.mojang.com"]);
	status += "  Session: " + convertStatus(statuses["sessionserver.mojang.com"]);

	return status;
}

void ServerStatus::updateStatusUI()
{
	m_statusRefresh->setChecked(false);
	MMC->statusChecker()->getStatusEntries();
	statusTimer.start(60 * 1000);
}

void ServerStatus::updateStatusFailedUI()
{
	m_statusRefresh->setChecked(false);
	StatusChanged();
	statusTimer.start(60 * 1000);
}
