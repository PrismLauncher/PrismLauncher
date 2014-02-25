#pragma once

#include <QDateTime>
#include <QString>
#include <memory>
#include <QIcon>

struct ScreenShot
{
	QIcon getImage();
	QIcon m_image;
	bool imageloaded = false;
	QDateTime timestamp;
	QString file;
	QString url;
	QString imgurId;
};

typedef std::shared_ptr<ScreenShot> ScreenshotPtr;
