#include "LaunchInteraction.h"
#include <auth/MojangAccountList.h>
#include "MultiMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/AccountSelectDialog.h"
#include "dialogs/ProgressDialog.h"
#include "dialogs/EditAccountDialog.h"
#include "ConsoleWindow.h"
#include "BuildConfig.h"
#include "JavaCommon.h"
#include "SettingsUI.h"
#include <QLineEdit>
#include <QInputDialog>
#include <tasks/Task.h>
#include <auth/YggdrasilTask.h>
#include <launch/steps/TextPrint.h>
#include <QStringList>

LaunchController::LaunchController(QObject *parent) : QObject(parent)
{
}

void LaunchController::launch()
{
	login();
}

// FIXME: minecraft specific
void LaunchController::login()
{
	if (!m_instance)
		return;

	JavaCommon::checkJVMArgs(m_instance->settings()->get("JvmArgs").toString(), m_parentWidget);

	// Find an account to use.
	std::shared_ptr<MojangAccountList> accounts = MMC->accounts();
	MojangAccountPtr account = accounts->activeAccount();
	if (accounts->count() <= 0)
	{
		// Tell the user they need to log in at least one account in order to play.
		auto reply = CustomMessageBox::selectable(
			m_parentWidget, tr("No Accounts"),
			tr("In order to play Minecraft, you must have at least one Mojang or Minecraft "
			   "account logged in to MultiMC."
			   "Would you like to open the account manager to add an account now?"),
			QMessageBox::Information, QMessageBox::Yes | QMessageBox::No)->exec();

		if (reply == QMessageBox::Yes)
		{
			// Open the account manager.
			SettingsUI::ShowPageDialog(MMC->globalSettingsPages(), m_parentWidget, "accounts");
		}
	}
	else if (account.get() == nullptr)
	{
		// If no default account is set, ask the user which one to use.
		AccountSelectDialog selectDialog(tr("Which account would you like to use?"),
										 AccountSelectDialog::GlobalDefaultCheckbox, m_parentWidget);

		selectDialog.exec();

		// Launch the instance with the selected account.
		account = selectDialog.selectedAccount();

		// If the user said to use the account as default, do that.
		if (selectDialog.useAsGlobalDefault() && account.get() != nullptr)
			accounts->setActiveAccount(account->username());
	}

	// if no account is selected, we bail
	if (!account.get())
		return;

	// we try empty password first :)
	QString password;
	// we loop until the user succeeds in logging in or gives up
	bool tryagain = true;
	// the failure. the default failure.
	const QString needLoginAgain = tr("Your account is currently not logged in. Please enter "
							"your password to log in again.");
	QString failReason = needLoginAgain;

	while (tryagain)
	{
		m_session = std::make_shared<AuthSession>();
		m_session->wants_online = m_online;
		auto task = account->login(m_session, password);
		if (task)
		{
			// We'll need to validate the access token to make sure the account
			// is still logged in.
			ProgressDialog progDialog(m_parentWidget);
			if (m_online)
				progDialog.setSkipButton(true, tr("Play Offline"));
			progDialog.execWithTask(task.get());
			if (!task->successful())
			{
				auto failReasonNew = task->failReason();
				if(failReasonNew == "Invalid token.")
				{
					failReason = needLoginAgain;
				}
				else failReason = failReasonNew;
			}
		}
		switch (m_session->status)
		{
		case AuthSession::Undetermined:
		{
			qCritical() << "Received undetermined session status during login. Bye.";
			tryagain = false;
			break;
		}
		case AuthSession::RequiresPassword:
		{
			EditAccountDialog passDialog(failReason, m_parentWidget, EditAccountDialog::PasswordField);
			auto username = m_session->username;
			auto chopN = [](QString toChop, int N) -> QString
			{
				if(toChop.size() > N)
				{
					auto left = toChop.left(N);
					left += QString("\u25CF").repeated(toChop.size() - N);
					return left;
				}
				return toChop;
			};

			if(username.contains('@'))
			{
				auto parts = username.split('@');
				auto mailbox = chopN(parts[0],3);
				QString domain = chopN(parts[1], 3);
				username = mailbox + '@' + domain;
			}
			passDialog.setUsername(username);
			if (passDialog.exec() == QDialog::Accepted)
			{
				password = passDialog.password();
			}
			else
			{
				tryagain = false;
			}
			break;
		}
		case AuthSession::PlayableOffline:
		{
			// we ask the user for a player name
			bool ok = false;
			QString usedname = m_session->player_name;
			QString name = QInputDialog::getText(m_parentWidget, tr("Player name"),
												 tr("Choose your offline mode player name."),
												 QLineEdit::Normal, m_session->player_name, &ok);
			if (!ok)
			{
				tryagain = false;
				break;
			}
			if (name.length())
			{
				usedname = name;
			}
			m_session->MakeOffline(usedname);
			// offline flavored game from here :3
		}
		case AuthSession::PlayableOnline:
		{
			launchInstance();
			tryagain = false;
		}
		}
	}
}

void LaunchController::launchInstance()
{
	Q_ASSERT_X(m_instance != NULL, "launchInstance", "instance is NULL");
	Q_ASSERT_X(m_session.get() != nullptr, "launchInstance", "session is NULL");

	if(!m_instance->reload())
	{
		QMessageBox::critical(m_parentWidget, tr("Error"), tr("Couldn't load the instance profile."));
		return;
	}

	m_launcher = m_instance->createLaunchTask(m_session);
	if (!m_launcher)
	{
		return;
	}

	if(m_parentWidget)
	{
		m_parentWidget->hide();
	}

	m_console = new ConsoleWindow(m_launcher);
	connect(m_console, &ConsoleWindow::isClosing, this, &LaunchController::instanceEnded);
	connect(m_launcher.get(), &LaunchTask::readyForLaunch, this, &LaunchController::readyForLaunch);

	m_launcher->prependStep(std::make_shared<TextPrint>(m_launcher.get(), "MultiMC version: " + BuildConfig.printableVersionString() + "\n\n", MessageLevel::MultiMC));
	m_launcher->start();
}

void LaunchController::readyForLaunch()
{
	if (!m_profiler)
	{
		m_launcher->proceed();
		return;
	}

	QString error;
	if (!m_profiler->check(&error))
	{
		m_launcher->abort();
		QMessageBox::critical(m_parentWidget, tr("Error"), tr("Couldn't start profiler: %1").arg(error));
		return;
	}
	BaseProfiler *profilerInstance = m_profiler->createProfiler(m_launcher->instance(), this);

	connect(profilerInstance, &BaseProfiler::readyToLaunch, [this](const QString & message)
	{
		QMessageBox msg;
		msg.setText(tr("The game launch is delayed until you press the "
						"button. This is the right time to setup the profiler, as the "
						"profiler server is running now.\n\n%1").arg(message));
		msg.setWindowTitle(tr("Waiting"));
		msg.setIcon(QMessageBox::Information);
		msg.addButton(tr("Launch"), QMessageBox::AcceptRole);
		msg.setModal(true);
		msg.exec();
		m_launcher->proceed();
	});
	connect(profilerInstance, &BaseProfiler::abortLaunch, [this](const QString & message)
	{
		QMessageBox msg;
		msg.setText(tr("Couldn't start the profiler: %1").arg(message));
		msg.setWindowTitle(tr("Error"));
		msg.setIcon(QMessageBox::Critical);
		msg.addButton(QMessageBox::Ok);
		msg.setModal(true);
		msg.exec();
		m_launcher->abort();
	});
	profilerInstance->beginProfiling(m_launcher);
}

void LaunchController::instanceEnded()
{
	if(m_parentWidget)
	{
		m_parentWidget->show();
	}
}
