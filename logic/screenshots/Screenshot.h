#pragma once

#include <QDateTime>
#include <QString>
#include <memory>

struct ScreenShot
{
	QDateTime timestamp;
	QString file;
	QString url;
	QString imgurId;
};

typedef std::shared_ptr<ScreenShot> ScreenshotPtr;
