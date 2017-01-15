#include "CustomTheme.h"
#include <QDir>
#include <Json.h>
#include <FileSystem.h>

const char * themeFile = "theme.json";
const char * styleFile = "themeStyle.css";

static bool readThemeJson(const QString &path, QPalette &palette, double &fadeAmount, QColor &fadeColor, QString &name, QString &widgets)
{
	QFileInfo pathInfo(path);
	if(pathInfo.exists() && pathInfo.isFile())
	{
		try
		{
			auto doc = Json::requireDocument(path, "Theme JSON file");
			const QJsonObject root = doc.object();
			name = Json::requireString(root, "name", "Theme name");
			widgets = Json::requireString(root, "widgets", "Qt widget theme");
			auto colorsRoot = Json::requireObject(root, "colors", "colors object");
			auto readColor = [&](QString colorName) -> QColor
			{
				auto colorValue = Json::ensureString(colorsRoot, colorName, QString());
				if(!colorValue.isEmpty())
				{
					QColor color(colorValue);
					if(!color.isValid())
					{
						qWarning() << "Color value" << colorValue << "for" << colorName << "was not recognized.";
						return QColor();
					}
					return color;
				}
				return QColor();
			};
			auto readAndSetColor = [&](QPalette::ColorRole role, QString colorName)
			{
				auto color = readColor(colorName);
				if(color.isValid())
				{
					palette.setColor(role, color);
				}
				else
				{
					qDebug() << "Color value for" << colorName << "was not present.";
				}
			};

			// palette
			readAndSetColor(QPalette::Window, "Window");
			readAndSetColor(QPalette::WindowText, "WindowText");
			readAndSetColor(QPalette::Base, "Base");
			readAndSetColor(QPalette::AlternateBase, "AlternateBase");
			readAndSetColor(QPalette::ToolTipBase, "ToolTipBase");
			readAndSetColor(QPalette::ToolTipText, "ToolTipText");
			readAndSetColor(QPalette::Text, "Text");
			readAndSetColor(QPalette::Button, "Button");
			readAndSetColor(QPalette::ButtonText, "ButtonText");
			readAndSetColor(QPalette::BrightText, "BrightText");
			readAndSetColor(QPalette::Link, "Link");
			readAndSetColor(QPalette::Highlight, "Highlight");
			readAndSetColor(QPalette::HighlightedText, "HighlightedText");

			//fade
			fadeColor = readColor("fadeColor");
			fadeAmount = Json::ensureDouble(colorsRoot, "fadeAmount", 0.5, "fade amount");

		}
		catch(Exception e)
		{
			qWarning() << "Couldn't load theme json: " << e.cause();
			return false;
		}
	}
	else
	{
		qDebug() << "No theme json present.";
		return false;
	}
	return true;
}

static bool writeThemeJson(const QString &path, const QPalette &palette, double fadeAmount, QColor fadeColor, QString name, QString widgets)
{
	QJsonObject rootObj;
	rootObj.insert("name", name);
	rootObj.insert("widgets", widgets);

	QJsonObject colorsObj;
	auto insertColor = [&](QPalette::ColorRole role, QString colorName)
	{
		colorsObj.insert(colorName, palette.color(role).name());
	};

	// palette
	insertColor(QPalette::Window, "Window");
	insertColor(QPalette::WindowText, "WindowText");
	insertColor(QPalette::Base, "Base");
	insertColor(QPalette::AlternateBase, "AlternateBase");
	insertColor(QPalette::ToolTipBase, "ToolTipBase");
	insertColor(QPalette::ToolTipText, "ToolTipText");
	insertColor(QPalette::Text, "Text");
	insertColor(QPalette::Button, "Button");
	insertColor(QPalette::ButtonText, "ButtonText");
	insertColor(QPalette::BrightText, "BrightText");
	insertColor(QPalette::Link, "Link");
	insertColor(QPalette::Highlight, "Highlight");
	insertColor(QPalette::HighlightedText, "HighlightedText");

	// fade
	colorsObj.insert("fadeColor", fadeColor.name());
	colorsObj.insert("fadeAmount", fadeAmount);

	rootObj.insert("colors", colorsObj);
	try
	{
		Json::write(rootObj, path);
		return true;
	}
	catch (Exception e)
	{
		qWarning() << "Failed to write theme json to" << path;
		return false;
	}
}

CustomTheme::CustomTheme(ITheme* baseTheme, QString folder)
{
	m_id = folder;
	QString path = FS::PathCombine("themes", m_id);
	QString pathResources = FS::PathCombine("themes", m_id, "resources");

	qDebug() << "Loading theme" << m_id;

	if(!FS::ensureFolderPathExists(path) || !FS::ensureFolderPathExists(pathResources))
	{
		qWarning() << "couldn't create folder for theme!";
		m_palette = baseTheme->colorScheme();
		m_styleSheet = baseTheme->appStyleSheet();
		return;
	}

	auto themeFilePath = FS::PathCombine(path, themeFile);

	m_palette = baseTheme->colorScheme();
	if (!readThemeJson(themeFilePath, m_palette, m_fadeAmount, m_fadeColor, m_name, m_widgets))
	{
		m_name = "Custom";
		m_palette = baseTheme->colorScheme();
		m_fadeColor = baseTheme->fadeColor();
		m_fadeAmount = baseTheme->fadeAmount();
		m_widgets = baseTheme->qtTheme();

		QFileInfo info(themeFilePath);
		if(!info.exists())
		{
			writeThemeJson(themeFilePath, m_palette, m_fadeAmount, m_fadeColor, "Custom", m_widgets);
		}
	}
	else
	{
		m_palette = fadeInactive(m_palette, m_fadeAmount, m_fadeColor);
	}

	auto cssFilePath = FS::PathCombine(path, styleFile);
	QFileInfo info (cssFilePath);
	if(info.isFile())
	{
		try
		{
			// TODO: validate css?
			m_styleSheet = QString::fromUtf8(FS::read(cssFilePath));
		}
		catch(Exception e)
		{
			qWarning() << "Couldn't load css:" << e.cause() << "from" << cssFilePath;
			m_styleSheet = baseTheme->appStyleSheet();
		}
	}
	else
	{
		qDebug() << "No theme css present.";
		m_styleSheet = baseTheme->appStyleSheet();
		try
		{
			FS::write(cssFilePath, m_styleSheet.toUtf8());
		}
		catch(Exception e)
		{
			qWarning() << "Couldn't write css:" << e.cause() << "to" << cssFilePath;
		}
	}
}

QStringList CustomTheme::searchPaths()
{
	return { FS::PathCombine("themes", m_id, "resources") };
}


QString CustomTheme::id()
{
	return m_id;
}

QString CustomTheme::name()
{
	return m_name;
}

bool CustomTheme::hasColorScheme()
{
	return true;
}

QPalette CustomTheme::colorScheme()
{
	return m_palette;
}

bool CustomTheme::hasStyleSheet()
{
	return true;
}

QString CustomTheme::appStyleSheet()
{
	return m_styleSheet;
}

double CustomTheme::fadeAmount()
{
	return m_fadeAmount;
}

QColor CustomTheme::fadeColor()
{
	return m_fadeColor;
}

QString CustomTheme::qtTheme()
{
	return m_widgets;
}
