#include "BaseUpdate.h"

BaseUpdate::BaseUpdate ( BaseInstance* inst, QObject* parent ) : Task ( parent )
{
	m_inst = inst;
}

void BaseUpdate::updateDownloadProgress(qint64 current, qint64 total)
{
	emit progress(current, total);
}