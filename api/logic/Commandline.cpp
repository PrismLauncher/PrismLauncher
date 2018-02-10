/* Copyright 2013-2018 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Commandline.h"

/**
 * @file libutil/src/cmdutils.cpp
 */

namespace Commandline
{

// commandline splitter
QStringList splitArgs(QString args)
{
	QStringList argv;
	QString current;
	bool escape = false;
	QChar inquotes;
	for (int i = 0; i < args.length(); i++)
	{
		QChar cchar = args.at(i);

		// \ escaped
		if (escape)
		{
			current += cchar;
			escape = false;
			// in "quotes"
		}
		else if (!inquotes.isNull())
		{
			if (cchar == '\\')
				escape = true;
			else if (cchar == inquotes)
				inquotes = 0;
			else
				current += cchar;
			// otherwise
		}
		else
		{
			if (cchar == ' ')
			{
				if (!current.isEmpty())
				{
					argv << current;
					current.clear();
				}
			}
			else if (cchar == '"' || cchar == '\'')
				inquotes = cchar;
			else
				current += cchar;
		}
	}
	if (!current.isEmpty())
		argv << current;
	return argv;
}

Parser::Parser(FlagStyle::Enum flagStyle, ArgumentStyle::Enum argStyle)
{
	m_flagStyle = flagStyle;
	m_argStyle = argStyle;
}

// styles setter/getter
void Parser::setArgumentStyle(ArgumentStyle::Enum style)
{
	m_argStyle = style;
}
ArgumentStyle::Enum Parser::argumentStyle()
{
	return m_argStyle;
}

void Parser::setFlagStyle(FlagStyle::Enum style)
{
	m_flagStyle = style;
}
FlagStyle::Enum Parser::flagStyle()
{
	return m_flagStyle;
}

// setup methods
void Parser::addSwitch(QString name, bool def)
{
	if (m_params.contains(name))
		throw "Name not unique";

	OptionDef *param = new OptionDef;
	param->type = otSwitch;
	param->name = name;
	param->metavar = QString("<%1>").arg(name);
	param->def = def;

	m_options[name] = param;
	m_params[name] = (CommonDef *)param;
	m_optionList.append(param);
}

void Parser::addOption(QString name, QVariant def)
{
	if (m_params.contains(name))
		throw "Name not unique";

	OptionDef *param = new OptionDef;
	param->type = otOption;
	param->name = name;
	param->metavar = QString("<%1>").arg(name);
	param->def = def;

	m_options[name] = param;
	m_params[name] = (CommonDef *)param;
	m_optionList.append(param);
}

void Parser::addArgument(QString name, bool required, QVariant def)
{
	if (m_params.contains(name))
		throw "Name not unique";

	PositionalDef *param = new PositionalDef;
	param->name = name;
	param->def = def;
	param->required = required;
	param->metavar = name;

	m_positionals.append(param);
	m_params[name] = (CommonDef *)param;
}

void Parser::addDocumentation(QString name, QString doc, QString metavar)
{
	if (!m_params.contains(name))
		throw "Name does not exist";

	CommonDef *param = m_params[name];
	param->doc = doc;
	if (!metavar.isNull())
		param->metavar = metavar;
}

void Parser::addShortOpt(QString name, QChar flag)
{
	if (!m_params.contains(name))
		throw "Name does not exist";
	if (!m_options.contains(name))
		throw "Name is not an Option or Swtich";

	OptionDef *param = m_options[name];
	m_flags[flag] = param;
	param->flag = flag;
}

// help methods
QString Parser::compileHelp(QString progName, int helpIndent, bool useFlags)
{
	QStringList help;
	help << compileUsage(progName, useFlags) << "\r\n";

	// positionals
	if (!m_positionals.isEmpty())
	{
		help << "\r\n";
		help << "Positional arguments:\r\n";
		QListIterator<PositionalDef *> it2(m_positionals);
		while (it2.hasNext())
		{
			PositionalDef *param = it2.next();
			help << "  " << param->metavar;
			help << " " << QString(helpIndent - param->metavar.length() - 1, ' ');
			help << param->doc << "\r\n";
		}
	}

	// Options
	if (!m_optionList.isEmpty())
	{
		help << "\r\n";
		QString optPrefix, flagPrefix;
		getPrefix(optPrefix, flagPrefix);

		help << "Options & Switches:\r\n";
		QListIterator<OptionDef *> it(m_optionList);
		while (it.hasNext())
		{
			OptionDef *option = it.next();
			help << "  ";
			int nameLength = optPrefix.length() + option->name.length();
			if (!option->flag.isNull())
			{
				nameLength += 3 + flagPrefix.length();
				help << flagPrefix << option->flag << ", ";
			}
			help << optPrefix << option->name;
			if (option->type == otOption)
			{
				QString arg = QString("%1%2").arg(
					((m_argStyle == ArgumentStyle::Equals) ? "=" : " "), option->metavar);
				nameLength += arg.length();
				help << arg;
			}
			help << " " << QString(helpIndent - nameLength - 1, ' ');
			help << option->doc << "\r\n";
		}
	}

	return help.join("");
}

QString Parser::compileUsage(QString progName, bool useFlags)
{
	QStringList usage;
	usage << "Usage: " << progName;

	QString optPrefix, flagPrefix;
	getPrefix(optPrefix, flagPrefix);

	// options
	QListIterator<OptionDef *> it(m_optionList);
	while (it.hasNext())
	{
		OptionDef *option = it.next();
		usage << " [";
		if (!option->flag.isNull() && useFlags)
			usage << flagPrefix << option->flag;
		else
			usage << optPrefix << option->name;
		if (option->type == otOption)
			usage << ((m_argStyle == ArgumentStyle::Equals) ? "=" : " ") << option->metavar;
		usage << "]";
	}

	// arguments
	QListIterator<PositionalDef *> it2(m_positionals);
	while (it2.hasNext())
	{
		PositionalDef *param = it2.next();
		usage << " " << (param->required ? "<" : "[");
		usage << param->metavar;
		usage << (param->required ? ">" : "]");
	}

	return usage.join("");
}

// parsing
QHash<QString, QVariant> Parser::parse(QStringList argv)
{
	QHash<QString, QVariant> map;

	QStringListIterator it(argv);
	QString programName = it.next();

	QString optionPrefix;
	QString flagPrefix;
	QListIterator<PositionalDef *> positionals(m_positionals);
	QStringList expecting;

	getPrefix(optionPrefix, flagPrefix);

	while (it.hasNext())
	{
		QString arg = it.next();

		if (!expecting.isEmpty())
		// we were expecting an argument
		{
			QString name = expecting.first();
/*
			if (map.contains(name))
				throw ParsingError(
					QString("Option %2%1 was given multiple times").arg(name, optionPrefix));
*/
			map[name] = QVariant(arg);

			expecting.removeFirst();
			continue;
		}

		if (arg.startsWith(optionPrefix))
		// we have an option
		{
			// qDebug("Found option %s", qPrintable(arg));

			QString name = arg.mid(optionPrefix.length());
			QString equals;

			if ((m_argStyle == ArgumentStyle::Equals ||
				 m_argStyle == ArgumentStyle::SpaceAndEquals) &&
				name.contains("="))
			{
				int i = name.indexOf("=");
				equals = name.mid(i + 1);
				name = name.left(i);
			}

			if (m_options.contains(name))
			{
				/*
				if (map.contains(name))
					throw ParsingError(QString("Option %2%1 was given multiple times")
										   .arg(name, optionPrefix));
*/
				OptionDef *option = m_options[name];
				if (option->type == otSwitch)
					map[name] = true;
				else // if (option->type == otOption)
				{
					if (m_argStyle == ArgumentStyle::Space)
						expecting.append(name);
					else if (!equals.isNull())
						map[name] = equals;
					else if (m_argStyle == ArgumentStyle::SpaceAndEquals)
						expecting.append(name);
					else
						throw ParsingError(QString("Option %2%1 reqires an argument.")
											   .arg(name, optionPrefix));
				}

				continue;
			}

			throw ParsingError(QString("Unknown Option %2%1").arg(name, optionPrefix));
		}

		if (arg.startsWith(flagPrefix))
		// we have (a) flag(s)
		{
			// qDebug("Found flags %s", qPrintable(arg));

			QString flags = arg.mid(flagPrefix.length());
			QString equals;

			if ((m_argStyle == ArgumentStyle::Equals ||
				 m_argStyle == ArgumentStyle::SpaceAndEquals) &&
				flags.contains("="))
			{
				int i = flags.indexOf("=");
				equals = flags.mid(i + 1);
				flags = flags.left(i);
			}

			for (int i = 0; i < flags.length(); i++)
			{
				QChar flag = flags.at(i);

				if (!m_flags.contains(flag))
					throw ParsingError(QString("Unknown flag %2%1").arg(flag, flagPrefix));

				OptionDef *option = m_flags[flag];
/*
				if (map.contains(option->name))
					throw ParsingError(QString("Option %2%1 was given multiple times")
										   .arg(option->name, optionPrefix));
*/
				if (option->type == otSwitch)
					map[option->name] = true;
				else // if (option->type == otOption)
				{
					if (m_argStyle == ArgumentStyle::Space)
						expecting.append(option->name);
					else if (!equals.isNull())
						if (i == flags.length() - 1)
							map[option->name] = equals;
						else
							throw ParsingError(QString("Flag %4%2 of Argument-requiring Option "
													   "%1 not last flag in %4%3")
												   .arg(option->name, flag, flags, flagPrefix));
					else if (m_argStyle == ArgumentStyle::SpaceAndEquals)
						expecting.append(option->name);
					else
						throw ParsingError(QString("Option %1 reqires an argument. (flag %3%2)")
											   .arg(option->name, flag, flagPrefix));
				}
			}

			continue;
		}

		// must be a positional argument
		if (!positionals.hasNext())
			throw ParsingError(QString("Don't know what to do with '%1'").arg(arg));

		PositionalDef *param = positionals.next();

		map[param->name] = arg;
	}

	// check if we're missing something
	if (!expecting.isEmpty())
		throw ParsingError(QString("Was still expecting arguments for %2%1").arg(
			expecting.join(QString(", ") + optionPrefix), optionPrefix));

	while (positionals.hasNext())
	{
		PositionalDef *param = positionals.next();
		if (param->required)
			throw ParsingError(
				QString("Missing required positional argument '%1'").arg(param->name));
		else
			map[param->name] = param->def;
	}

	// fill out gaps
	QListIterator<OptionDef *> iter(m_optionList);
	while (iter.hasNext())
	{
		OptionDef *option = iter.next();
		if (!map.contains(option->name))
			map[option->name] = option->def;
	}

	return map;
}

// clear defs
void Parser::clear()
{
	m_flags.clear();
	m_params.clear();
	m_options.clear();

	QMutableListIterator<OptionDef *> it(m_optionList);
	while (it.hasNext())
	{
		OptionDef *option = it.next();
		it.remove();
		delete option;
	}

	QMutableListIterator<PositionalDef *> it2(m_positionals);
	while (it2.hasNext())
	{
		PositionalDef *arg = it2.next();
		it2.remove();
		delete arg;
	}
}

// Destructor
Parser::~Parser()
{
	clear();
}

// getPrefix
void Parser::getPrefix(QString &opt, QString &flag)
{
	if (m_flagStyle == FlagStyle::Windows)
		opt = flag = "/";
	else if (m_flagStyle == FlagStyle::Unix)
		opt = flag = "-";
	// else if (m_flagStyle == FlagStyle::GNU)
	else
	{
		opt = "--";
		flag = "-";
	}
}

// ParsingError
ParsingError::ParsingError(const QString &what) : std::runtime_error(what.toStdString())
{
}
}