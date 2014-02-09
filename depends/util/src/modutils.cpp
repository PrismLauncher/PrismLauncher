#include "include/modutils.h"

#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Util::Version::Version(const QString &str) : m_string(str)
{
}

bool Util::Version::operator<(const Version &other) const
{
	QStringList parts1 = m_string.split('.');
	QStringList parts2 = other.m_string.split('.');

	while (!parts1.isEmpty() && !parts2.isEmpty())
	{
		QString part1 = parts1.isEmpty() ? "0" : parts1.takeFirst();
		QString part2 = parts2.isEmpty() ? "0" : parts2.takeFirst();
		bool ok1 = false;
		bool ok2 = false;
		int int1 = part1.toInt(&ok1);
		int int2 = part2.toInt(&ok2);
		if (ok1 && ok2)
		{
			if (int1 == int2)
			{
				continue;
			}
			else
			{
				return int1 < int2;
			}
		}
		else
		{
			if (part1 == part2)
			{
				continue;
			}
			else
			{
				return part1 < part2;
			}
		}
	}

	return false;
}
bool Util::Version::operator<=(const Util::Version &other) const
{
	return *this < other || *this == other;
}
bool Util::Version::operator>(const Version &other) const
{
	QStringList parts1 = m_string.split('.');
	QStringList parts2 = other.m_string.split('.');

	while (!parts1.isEmpty() && !parts2.isEmpty())
	{
		QString part1 = parts1.isEmpty() ? "0" : parts1.takeFirst();
		QString part2 = parts2.isEmpty() ? "0" : parts2.takeFirst();
		bool ok1 = false;
		bool ok2 = false;
		int int1 = part1.toInt(&ok1);
		int int2 = part2.toInt(&ok2);
		if (ok1 && ok2)
		{
			if (int1 == int2)
			{
				continue;
			}
			else
			{
				return int1 > int2;
			}
		}
		else
		{
			if (part1 == part2)
			{
				continue;
			}
			else
			{
				return part1 > part2;
			}
		}
	}

	return false;
}
bool Util::Version::operator==(const Version &other) const
{
	QStringList parts1 = m_string.split('.');
	QStringList parts2 = other.m_string.split('.');

	while (!parts1.isEmpty() && !parts2.isEmpty())
	{
		QString part1 = parts1.isEmpty() ? "0" : parts1.takeFirst();
		QString part2 = parts2.isEmpty() ? "0" : parts2.takeFirst();
		bool ok1 = false;
		bool ok2 = false;
		int int1 = part1.toInt(&ok1);
		int int2 = part2.toInt(&ok2);
		if (ok1 && ok2)
		{
			if (int1 == int2)
			{
				continue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (part1 == part2)
			{
				continue;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}
bool Util::Version::operator!=(const Version &other) const
{
	return !operator==(other);
}

QUrl Util::expandQMURL(const QString &in)
{
	QUrl inUrl(in);
	if (inUrl.scheme() == "github")
	{
		// needed because QUrl makes the host all lower cases
		const QString repo = in.mid(in.indexOf(inUrl.host(), 0, Qt::CaseInsensitive), inUrl.host().size());
		QUrl out;
		out.setScheme("https");
		out.setHost("raw.github.com");
		out.setPath(QString("/%1/%2/%3%4")
						.arg(inUrl.userInfo(), repo,
							 inUrl.fragment().isEmpty() ? "master" : inUrl.fragment(), inUrl.path()));
		return out;
	}
	else if (inUrl.scheme() == "mcf")
	{
		QUrl out;
		out.setScheme("http");
		out.setHost("www.minecraftforum.net");
		out.setPath(QString("/topic/%1-").arg(inUrl.path()));
		return out;
	}
	else
	{
		return in;
	}
}

bool Util::versionIsInInterval(const QString &version, const QString &interval)
{
	if (interval.isEmpty() || version == interval)
	{
		return true;
	}

	// Interval notation is used
	QRegularExpression exp(
		"(?<start>[\\[\\]\\(\\)])(?<bottom>.*?)(,(?<top>.*?))?(?<end>[\\[\\]\\(\\)])");
	QRegularExpressionMatch match = exp.match(interval);
	if (match.hasMatch())
	{
		const QChar start = match.captured("start").at(0);
		const QChar end = match.captured("end").at(0);
		const QString bottom = match.captured("bottom");
		const QString top = match.captured("top");

		// check if in range (bottom)
		if (!bottom.isEmpty())
		{
			if ((start == '[') && !(version >= bottom))
			{
				return false;
			}
			else if ((start == '(') && !(version > bottom))
			{
				return false;
			}
		}

		// check if in range (top)
		if (!top.isEmpty())
		{
			if ((end == ']') && !(version <= top))
			{
				return false;
			}
			else if ((end == ')') && !(version < top))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

