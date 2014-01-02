#include "AssetsMigrateTask.h"
#include "MultiMC.h"
#include "logger/QsLog.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDirIterator>
#include <QCryptographicHash>
#include "gui/dialogs/CustomMessageBox.h"
#include <QDesktopServices>

AssetsMigrateTask::AssetsMigrateTask(int expected, QObject *parent)
	: Task(parent)
{
	this->m_expected = expected;
}

void AssetsMigrateTask::executeTask()
{
	this->setStatus(tr("Migrating legacy assets..."));
	this->setProgress(0);

	QDir assets_dir("assets");
	if (!assets_dir.exists())
	{
		emitFailed("Assets directory didn't exist");
		return;
	}
	assets_dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
	int base_length = assets_dir.path().length();

	QList<QString> blacklist = {"indexes", "objects", "virtual"};

	if (!assets_dir.exists("objects"))
		assets_dir.mkdir("objects");
	QDir objects_dir("assets/objects");

	QDirIterator iterator(assets_dir, QDirIterator::Subdirectories);
	int successes = 0;
	int failures = 0;
	while (iterator.hasNext())
	{
		QString currentDir = iterator.next();
		currentDir = currentDir.remove(0, base_length + 1);

		bool ignore = false;
		for (QString blacklisted : blacklist)
		{
			if (currentDir.startsWith(blacklisted))
				ignore = true;
		}

		if (!iterator.fileInfo().isDir() && !ignore)
		{
			QString filename = iterator.filePath();

			QFile input(filename);
			input.open(QIODevice::ReadOnly | QIODevice::WriteOnly);
			QString sha1sum =
				QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha1)
					.toHex()
					.constData();

			QString object_name = filename.remove(0, base_length + 1);
			QLOG_DEBUG() << "Processing" << object_name << ":" << sha1sum << input.size();

			QString object_tlk = sha1sum.left(2);
			QString object_tlk_dir = objects_dir.path() + "/" + object_tlk;

			QDir tlk_dir(object_tlk_dir);
			if (!tlk_dir.exists())
				objects_dir.mkdir(object_tlk);

			QString new_filename = tlk_dir.path() + "/" + sha1sum;
			QFile new_object(new_filename);
			if (!new_object.exists())
			{
				bool rename_success = input.rename(new_filename);
				QLOG_DEBUG() << " Doesn't exist, copying to" << new_filename << ":"
							 << QString::number(rename_success);
				if (rename_success)
					successes++;
				else
					failures++;
			}
			else
			{
				input.remove();
				QLOG_DEBUG() << " Already exists, deleting original and not copying.";
			}

			this->setProgress(100 * ((successes + failures) / (float) this->m_expected));
		}
	}

	if (successes + failures == 0)
	{
		this->setProgress(100);
		QLOG_DEBUG() << "No legacy assets needed importing.";
	}
	else
	{
		QLOG_DEBUG() << "Finished copying legacy assets:" << successes << "successes and"
					 << failures << "failures.";

		this->setStatus("Cleaning up legacy assets...");
		this->setProgress(100);

		QDirIterator cleanup_iterator(assets_dir);

		while (cleanup_iterator.hasNext())
		{
			QString currentDir = cleanup_iterator.next();
			currentDir = currentDir.remove(0, base_length + 1);

			bool ignore = false;
			for (QString blacklisted : blacklist)
			{
				if (currentDir.startsWith(blacklisted))
					ignore = true;
			}

			if (cleanup_iterator.fileInfo().isDir() && !ignore)
			{
				QString path = cleanup_iterator.filePath();
				QDir folder(path);

				QLOG_DEBUG() << "Cleaning up legacy assets folder:" << path;

				folder.removeRecursively();
			}
		}
	}

	if(failures > 0)
	{
		emitFailed(QString("Failed to migrate %1 legacy assets").arg(failures));
	}
	else
	{
		emitSucceeded();
	}
}

