#include "ServerStatus.h"
#include "LineSeparator.h"
#include "IconLabel.h"
#include "status/StatusChecker.h"

#include "MultiMC.h"

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QMap>
#include <QToolButton>
#include <QAction>
#include <QDesktopServices>

class ClickableLabel : public QLabel
{
	Q_OBJECT
public:
	ClickableLabel(QWidget *parent) : QLabel(parent)
	{
		setCursor(Qt::PointingHandCursor);
	}

	~ClickableLabel(){};

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent *event)
	{
		emit clicked();
	}
};

class ClickableIconLabel : public IconLabel
{
	Q_OBJECT
public:
	ClickableIconLabel(QWidget *parent, QIcon icon, QSize size) : IconLabel(parent, icon, size)
	{
		setCursor(Qt::PointingHandCursor);
	}

	~ClickableIconLabel(){};

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent *event)
	{
		emit clicked();
	}
};

ServerStatus::ServerStatus(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	goodIcon = MMC->getThemedIcon("status-good");
	yellowIcon = MMC->getThemedIcon("status-yellow");
	badIcon = MMC->getThemedIcon("status-bad");

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
	m_statusRefresh->setIcon(MMC->getThemedIcon("refresh"));
	layout->addWidget(m_statusRefresh);

	setLayout(layout);

	// Start status checker
	m_statusChecker.reset(new StatusChecker());
	{
		auto reloader = m_statusChecker.get();
		connect(reloader, &StatusChecker::statusChanged, this, &ServerStatus::StatusChanged);
		connect(reloader, &StatusChecker::statusLoading, this, &ServerStatus::StatusReloading);
		connect(m_statusRefresh, &QAbstractButton::clicked, this, &ServerStatus::reloadStatus);
		m_statusChecker->startTimer(60000);
		reloadStatus();
	}
}

ServerStatus::~ServerStatus()
{
}

void ServerStatus::reloadStatus()
{
	m_statusChecker->reloadStatus();
}

void ServerStatus::addLine()
{
	layout->addWidget(new LineSeparator(this, Qt::Vertical));
}

void ServerStatus::addStatus(QString key, QString name)
{
	{
		auto label = new ClickableIconLabel(this, badIcon, QSize(16, 16));
		label->setToolTip(key);
		serverLabels[key] = label;
		layout->addWidget(label);
		connect(label,SIGNAL(clicked()),SLOT(clicked()));
	}
	{
		auto label = new ClickableLabel(this);
		label->setText(name);
		label->setToolTip(key);
		layout->addWidget(label);
		connect(label,SIGNAL(clicked()),SLOT(clicked()));
	}
}

void ServerStatus::clicked()
{
	QDesktopServices::openUrl(QUrl("https://help.mojang.com/"));
}

void ServerStatus::setStatus(QString key, int value)
{
	if (!serverLabels.contains(key))
		return;
	IconLabel *label = serverLabels[key];
	switch(value)
	{
		case 0:
			label->setIcon(goodIcon);
			break;
		case 1:
			label->setIcon(yellowIcon);
			break;
		default:
		case 2:
			label->setIcon(badIcon);
			break;
	}
}

void ServerStatus::StatusChanged(const QMap<QString, QString> statusEntries)
{
	auto convertStatus = [&](QString status)->int
	{
		if (status == "green")
			return 0;
		else if (status == "yellow")
			return 1;
		else if (status == "red")
			return 2;
		return 2;
	}
	;
	auto iter = statusEntries.begin();
	while (iter != statusEntries.end())
	{
		QString key = iter.key();
		auto value = convertStatus(iter.value());
		setStatus(key, value);
		iter++;
	}
}

void ServerStatus::StatusReloading(bool is_reloading)
{
	m_statusRefresh->setChecked(is_reloading);
}

#include "ServerStatus.moc"
