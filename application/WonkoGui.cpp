#include "WonkoGui.h"

#include "dialogs/ProgressDialog.h"
#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersionList.h"
#include "wonko/WonkoVersion.h"
#include "Env.h"

WonkoIndexPtr Wonko::ensureIndexLoaded(QWidget *parent)
{
	if (!ENV.wonkoIndex()->isLocalLoaded())
	{
		ProgressDialog(parent).execWithTask(ENV.wonkoIndex()->localUpdateTask());
		if (!ENV.wonkoIndex()->isRemoteLoaded() && ENV.wonkoIndex()->lists().size() == 0)
		{
			ProgressDialog(parent).execWithTask(ENV.wonkoIndex()->remoteUpdateTask());
		}
	}
	return ENV.wonkoIndex();
}

WonkoVersionListPtr Wonko::ensureVersionListExists(const QString &uid, QWidget *parent)
{
	ensureIndexLoaded(parent);
	if (!ENV.wonkoIndex()->isRemoteLoaded() && !ENV.wonkoIndex()->hasUid(uid))
	{
		ProgressDialog(parent).execWithTask(ENV.wonkoIndex()->remoteUpdateTask());
	}
	return ENV.wonkoIndex()->getList(uid);
}
WonkoVersionListPtr Wonko::ensureVersionListLoaded(const QString &uid, QWidget *parent)
{
	WonkoVersionListPtr list = ensureVersionListExists(uid, parent);
	if (!list)
	{
		return nullptr;
	}
	if (!list->isLocalLoaded())
	{
		ProgressDialog(parent).execWithTask(list->localUpdateTask());
		if (!list->isLocalLoaded())
		{
			ProgressDialog(parent).execWithTask(list->remoteUpdateTask());
		}
	}
	return list->isComplete() ? list : nullptr;
}

WonkoVersionPtr Wonko::ensureVersionExists(const QString &uid, const QString &version, QWidget *parent)
{
	WonkoVersionListPtr list = ensureVersionListLoaded(uid, parent);
	if (!list)
	{
		return nullptr;
	}
	return list->getVersion(version);
}
WonkoVersionPtr Wonko::ensureVersionLoaded(const QString &uid, const QString &version, QWidget *parent, const UpdateType update)
{
	WonkoVersionPtr vptr = ensureVersionExists(uid, version, parent);
	if (!vptr)
	{
		return nullptr;
	}
	if (!vptr->isLocalLoaded() || update == AlwaysUpdate)
	{
		ProgressDialog(parent).execWithTask(vptr->localUpdateTask());
		if (!vptr->isLocalLoaded() || update == AlwaysUpdate)
		{
			ProgressDialog(parent).execWithTask(vptr->remoteUpdateTask());
		}
	}
	return vptr->isComplete() ? vptr : nullptr;
}
