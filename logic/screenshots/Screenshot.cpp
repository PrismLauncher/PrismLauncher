#include "Screenshot.h"
#include <QImage>
#include <QIcon>
QIcon ScreenShot::getImage()
{
	if(!imageloaded)
	{
		QImage image(file);
		QImage thumbnail = image.scaledToWidth(256, Qt::SmoothTransformation);
		m_image = QIcon(QPixmap::fromImage(thumbnail));
		imageloaded = true;
	}
	return m_image;
}
