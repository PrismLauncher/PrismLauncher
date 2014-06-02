#include "ServerStatus.h"
#include "LineSeparator.h"
#include "IconLabel.h"
#include "logic/status/StatusChecker.h"

#include "MultiMC.h"

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QMap>
#include <QToolButton>
#include <QAction>

ServerStatus::ServerStatus(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	goodIcon = QIcon::fromTheme("status-good");
	badIcon = QIcon::fromTheme("status-bad");

	addStatus("minecraft.net", tr("Web"));
	addLine();
	addStatus("account.mojang.com", tr("Account"));
	addLine();
	addStatus("skins.minecraft.net", tr("Skins"));
	addLine();
	addStatus("authserver.mojang.com", tr("Auth"));
	addLine();
	addStatus("sessionserver.mojang.com", tr("Session"));

	m_statusRefresh = new QToolButton(this);
	m_statusRefresh->setCheckable(true);
	m_statusRefresh->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_statusRefresh->setIcon(QIcon::fromTheme("refresh"));
	layout->addWidget(m_statusRefresh);

	setLayout(layout);

	// Start status checker
	{
		auto reloader = MMC->statusChecker().get();
		connect(reloader, &StatusChecker::statusChanged, this, &ServerStatus::StatusChanged);
		connect(reloader, &StatusChecker::statusLoading, this, &ServerStatus::StatusReloading);
		connect(m_statusRefresh, &QAbstractButton::clicked, this, &ServerStatus::reloadStatus);
		MMC->statusChecker()->startTimer(60000);
		reloadStatus();
	}
}

ServerStatus::~ServerStatus()
{
}

void ServerStatus::reloadStatus()
{
	MMC->statusChecker()->reloadStatus();
}

void ServerStatus::addLine()
{
	layout->addWidget(new LineSeparator(this, Qt::Vertical));
}

void ServerStatus::addStatus(QString key, QString name)
{
	{
		auto label = new IconLabel(this, badIcon, QSize(16, 16));
		label->setToolTip(key);
		serverLabels[key] = label;
		layout->addWidget(label);
	}
	{
		auto label = new QLabel(this);
		label->setText(name);
		label->setToolTip(key);
		layout->addWidget(label);
	}
}

void ServerStatus::setStatus(QString key, bool value)
{
	if (!serverLabels.contains(key))
		return;
	IconLabel *label = serverLabels[key];
	label->setIcon(value ? goodIcon : badIcon);
}

void ServerStatus::StatusChanged(const QMap<QString, QString> statusEntries)
{
	auto convertStatus = [&](QString status)->bool
	{
		if (status == "green")
			return true;
		else if (status == "yellow")
			return false;
		else if (status == "red")
			return false;
		return false;
	}
	;
	auto iter = statusEntries.begin();
	while (iter != statusEntries.end())
	{
		QString key = iter.key();
		bool value = convertStatus(iter.value());
		setStatus(key, value);
		iter++;
	}
}

void ServerStatus::StatusReloading(bool is_reloading)
{
	m_statusRefresh->setChecked(is_reloading);
}
