#include "BaseUpdate.h"

BaseUpdate::BaseUpdate ( BaseInstance* inst, QObject* parent ) : Task ( parent )
{
	m_inst = inst;
}

void BaseUpdate::error ( const QString& msg )
{
	emit gameUpdateError(msg);
}

void BaseUpdate::updateDownloadProgress(qint64 current, qint64 total)
{
	// The progress on the current file is current / total
	float currentDLProgress = (float) current / (float) total;
	setProgress((int)(currentDLProgress * 100)); // convert to percentage
}