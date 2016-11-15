#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session): LaunchStep(parent)
{
	if(session->status == AuthSession::Status::PlayableOnline)
	{
		m_account = session->m_accountPtr;
	}
}

void ClaimAccount::executeTask()
{
	if(m_account)
	{
		lock.reset(new UseLock(m_account));
		emitSucceeded();
	}
}

void ClaimAccount::finalize()
{
	lock.reset();
}
