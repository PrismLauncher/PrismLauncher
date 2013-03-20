#include "iconcache.h"
#include <QMap>
#include <QWebView>
#include <QWebFrame>
#include <QEventLoop>
#include <QWebElement>

IconCache* IconCache::m_Instance = 0;
QMutex IconCache::mutex;
#define MAX_SIZE 1024

class Private : public QWebView
{
	Q_OBJECT

public:
	QString name;
	QSize size;
	QMap<QString, QIcon> icons;

public:
	Private()
	{
		connect(this, SIGNAL(loadFinished(bool)), this, SLOT(svgLoaded(bool)));
		setFixedSize(MAX_SIZE, MAX_SIZE);

		QPalette pal = palette();
		pal.setColor(QPalette::Base, Qt::transparent);
		setPalette(pal);
		setAttribute(Qt::WA_OpaquePaintEvent, false);
		size = QSize(128,128);
	}
	void renderSVGIcon(QString name);

signals:
	void svgRendered();

private slots:
	void svgLoaded(bool ok);
};

void Private::svgLoaded(bool ok)
{
	if (!ok)
	{
		emit svgRendered();
		return;
	}
	// check for SVG root tag
	QString root = page()->currentFrame()->documentElement().tagName();
	if (root.compare("svg", Qt::CaseInsensitive) != 0)
	{
		emit svgRendered();
		return;
	}

    // get the size of the svg image, check if it's valid
	auto elem = page()->currentFrame()->documentElement();
	double width = elem.attribute("width").toDouble();
	double height = elem.attribute("height").toDouble();
    if (width == 0.0 || height == 0.0 || width == MAX_SIZE || height == MAX_SIZE)
	{
		emit svgRendered();
		return;
	}

	// create the target surface
	QSize t = size.isValid() ? size : QSize(width, height);
	QImage img(t, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);

	// prepare the painter, scale to required size
	QPainter p(&img);
	if(size.isValid())
	{
		p.scale(size.width() / width, size.height() / height);
	}

	// the best quality
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);

	page()->mainFrame()->render(&p,QWebFrame::ContentsLayer);
	p.end();

	icons[name] = QIcon(QPixmap::fromImage(img));
	emit svgRendered();
}

void Private::renderSVGIcon ( QString name )
{
	// use event loop to wait for signal
	QEventLoop loop;
	this->name = name;
	QString prefix = "qrc:/icons/instances/";
	QObject::connect(this, SIGNAL(svgRendered()), &loop, SLOT(quit()));
	load(QUrl(prefix + name));
	loop.exec();
}

IconCache::IconCache():d(new Private())
{
}

QIcon IconCache::getIcon ( QString name )
{
	if(name == "default")
		name = "infinity";
	{
		auto iter = d->icons.find(name);
		if(iter != d->icons.end())
			return *iter;
	}
	d->renderSVGIcon(name);
	auto iter = d->icons.find(name);
	if(iter != d->icons.end())
		return *iter;
	
	// Fallback for icons that don't exist.
	QString path = ":/icons/instances/infinity";
	//path += name;
	d->icons[name] = QIcon(path);
	return d->icons[name];
}

#include "iconcache.moc"