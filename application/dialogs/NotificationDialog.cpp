#include "NotificationDialog.h"
#include "ui_NotificationDialog.h"

#include <QTimerEvent>
#include <QStyle>

NotificationDialog::NotificationDialog(const NotificationChecker::NotificationEntry &entry, QWidget *parent) :
	QDialog(parent, Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint),
	ui(new Ui::NotificationDialog)
{
	ui->setupUi(this);

	QStyle::StandardPixmap icon;
	switch (entry.type)
	{
	case NotificationChecker::NotificationEntry::Critical:
		icon = QStyle::SP_MessageBoxCritical;
		break;
	case NotificationChecker::NotificationEntry::Warning:
		icon = QStyle::SP_MessageBoxWarning;
		break;
	default:
	case NotificationChecker::NotificationEntry::Information:
		icon = QStyle::SP_MessageBoxInformation;
		break;
	}
	ui->iconLabel->setPixmap(style()->standardPixmap(icon, 0, this));
	ui->messageLabel->setText(entry.message);

	m_dontShowAgainText = tr("Don't show again");
	m_closeText = tr("Close");

	ui->dontShowAgainBtn->setText(m_dontShowAgainText + QString(" (%1)").arg(m_dontShowAgainTime));
	ui->closeBtn->setText(m_closeText + QString(" (%1)").arg(m_closeTime));

	startTimer(1000);
}

NotificationDialog::~NotificationDialog()
{
	delete ui;
}

void NotificationDialog::timerEvent(QTimerEvent *event)
{
	if (m_dontShowAgainTime > 0)
	{
		m_dontShowAgainTime--;
		if (m_dontShowAgainTime == 0)
		{
			ui->dontShowAgainBtn->setText(m_dontShowAgainText);
			ui->dontShowAgainBtn->setEnabled(true);
		}
		else
		{
			ui->dontShowAgainBtn->setText(m_dontShowAgainText + QString(" (%1)").arg(m_dontShowAgainTime));
		}
	}
	if (m_closeTime > 0)
	{
		m_closeTime--;
		if (m_closeTime == 0)
		{
			ui->closeBtn->setText(m_closeText);
			ui->closeBtn->setEnabled(true);
		}
		else
		{
			ui->closeBtn->setText(m_closeText + QString(" (%1)").arg(m_closeTime));
		}
	}

	if (m_closeTime == 0 && m_dontShowAgainTime == 0)
	{
		killTimer(event->timerId());
	}
}

void NotificationDialog::on_dontShowAgainBtn_clicked()
{
	done(DontShowAgain);
}
void NotificationDialog::on_closeBtn_clicked()
{
	done(Normal);
}
