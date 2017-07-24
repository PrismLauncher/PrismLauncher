#include "ProfileUtils.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/OneSixVersionFormat.h"
#include "Json.h"
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSaveFile>

namespace ProfileUtils
{

static const int currentOrderFileVersion = 1;

bool writeOverrideOrders(QString path, const PatchOrder &order)
{
	QJsonObject obj;
	obj.insert("version", currentOrderFileVersion);
	QJsonArray orderArray;
	for(auto str: order)
	{
		orderArray.append(str);
	}
	obj.insert("order", orderArray);
	QSaveFile orderFile(path);
	if (!orderFile.open(QFile::WriteOnly))
	{
		qCritical() << "Couldn't open" << orderFile.fileName()
					 << "for writing:" << orderFile.errorString();
		return false;
	}
	auto data = QJsonDocument(obj).toJson(QJsonDocument::Indented);
	if(orderFile.write(data) != data.size())
	{
		qCritical() << "Couldn't write all the data into" << orderFile.fileName()
					 << "because:" << orderFile.errorString();
		return false;
	}
	if(!orderFile.commit())
	{
		qCritical() << "Couldn't save" << orderFile.fileName()
					 << "because:" << orderFile.errorString();
	}
	return true;
}

bool readOverrideOrders(QString path, PatchOrder &order)
{
	QFile orderFile(path);
	if (!orderFile.exists())
	{
		qWarning() << "Order file doesn't exist. Ignoring.";
		return false;
	}
	if (!orderFile.open(QFile::ReadOnly))
	{
		qCritical() << "Couldn't open" << orderFile.fileName()
					 << " for reading:" << orderFile.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and it's valid JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		qCritical() << "Couldn't parse" << orderFile.fileName() << ":" << error.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and then read it and process it if all above is true.
	try
	{
		auto obj = Json::requireObject(doc);
		// check order file version.
		auto version = Json::requireInteger(obj.value("version"));
		if (version != currentOrderFileVersion)
		{
			throw JSONValidationError(QObject::tr("Invalid order file version, expected %1")
										  .arg(currentOrderFileVersion));
		}
		auto orderArray = Json::requireArray(obj.value("order"));
		for(auto item: orderArray)
		{
			order.append(Json::requireString(item));
		}
	}
	catch (JSONValidationError &err)
	{
		qCritical() << "Couldn't parse" << orderFile.fileName() << ": bad file format";
		qWarning() << "Ignoring overriden order";
		order.clear();
		return false;
	}
	return true;
}

static VersionFilePtr createErrorVersionFile(QString fileId, QString filepath, QString error)
{
	auto outError = std::make_shared<VersionFile>();
	outError->uid = outError->name = fileId;
	// outError->filename = filepath;
	outError->addProblem(ProblemSeverity::Error, error);
	return outError;
}

static VersionFilePtr guardedParseJson(const QJsonDocument & doc,const QString &fileId,const QString &filepath,const bool &requireOrder)
{
	try
	{
		return OneSixVersionFormat::versionFileFromJson(doc, filepath, requireOrder);
	}
	catch (Exception & e)
	{
		return createErrorVersionFile(fileId, filepath, e.cause());
	}
}

VersionFilePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		auto errorStr = QObject::tr("Unable to open the version file %1: %2.").arg(fileInfo.fileName(), file.errorString());
		return createErrorVersionFile(fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), errorStr);
	}
	QJsonParseError error;
	auto data = file.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);
	file.close();
	if (error.error != QJsonParseError::NoError)
	{
		int line = 1;
		int column = 0;
		for(int i = 0; i < error.offset; i++)
		{
			if(data[i] == '\n')
			{
				line++;
				column = 0;
				continue;
			}
			column++;
		}
		auto errorStr = QObject::tr("Unable to process the version file %1: %2 at line %3 column %4.")
				.arg(fileInfo.fileName(), error.errorString())
				.arg(line).arg(column);
		return createErrorVersionFile(fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), errorStr);
	}
	return guardedParseJson(doc, fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), requireOrder);
}

VersionFilePtr parseBinaryJsonFile(const QFileInfo &fileInfo)
{
	QFile file(fileInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
	{
		auto errorStr = QObject::tr("Unable to open the version file %1: %2.").arg(fileInfo.fileName(), file.errorString());
		return createErrorVersionFile(fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), errorStr);
	}
	QJsonDocument doc = QJsonDocument::fromBinaryData(file.readAll());
	file.close();
	if (doc.isNull())
	{
		file.remove();
		throw JSONValidationError(QObject::tr("Unable to process the version file %1.").arg(fileInfo.fileName()));
	}
	return guardedParseJson(doc, fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), false);
}

void removeLwjglFromPatch(VersionFilePtr patch)
{
	auto filter = [](QList<LibraryPtr>& libs)
	{
		QList<LibraryPtr> filteredLibs;
		for (auto lib : libs)
		{
			if (!g_VersionFilterData.lwjglWhitelist.contains(lib->artifactPrefix()))
			{
				filteredLibs.append(lib);
			}
		}
		libs = filteredLibs;
	};
	filter(patch->libraries);
}
}
