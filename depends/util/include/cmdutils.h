/* Copyright 2013 MultiMC Contributors
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

#ifndef CMDUTILS_H
#define CMDUTILS_H

#include <exception>
#include <stdexcept>

#include <QString>
#include <QVariant>
#include <QHash>
#include <QStringList>

#include "libutil_config.h"

/**
 * @file libutil/include/cmdutils.h
 * @brief commandline parsing and processing utilities
 */

namespace Util {
namespace Commandline {

/**
 * @brief split a string into argv items like a shell would do
 * @param args the argument string
 * @return a QStringList containing all arguments
 */
LIBUTIL_EXPORT QStringList splitArgs(QString args);

/**
 * @brief The FlagStyle enum
 * Specifies how flags are decorated
 */

namespace FlagStyle
{
enum Enum
{
	GNU,     /**< --option and -o (GNU Style) */
	Unix,    /**< -option and -o  (Unix Style) */
	Windows, /**< /option and /o  (Windows Style) */
#ifdef Q_OS_WIN32
	Default = Windows
#else
	Default = GNU
#endif
};
}

/**
 * @brief The ArgumentStyle enum
 */
namespace ArgumentStyle 
{
enum Enum
{
	Space,          /**< --option=value */
	Equals,         /**< --option value */
	SpaceAndEquals, /**< --option[= ]value */
#ifdef Q_OS_WIN32
	Default = Equals
#else
	Default = SpaceAndEquals
#endif
};
}

/**
 * @brief The ParsingError class
 */
class LIBUTIL_EXPORT ParsingError : public std::runtime_error
{
public:
	ParsingError(const QString &what);
};

/**
 * @brief The Parser class
 */
class LIBUTIL_EXPORT Parser
{
public:
	/**
	 * @brief Parser constructor
	 * @param flagStyle the FlagStyle to use in this Parser
	 * @param argStyle the ArgumentStyle to use in this Parser
	 */
	Parser(FlagStyle::Enum flagStyle = FlagStyle::Default, 
		   ArgumentStyle::Enum argStyle = ArgumentStyle::Default);
	
	/**
	 * @brief set the flag style
	 * @param style
	 */
	void setFlagStyle(FlagStyle::Enum style);
	
	/**
	 * @brief get the flag style
	 * @return
	 */
	FlagStyle::Enum flagStyle();
	
	/**
	 * @brief set the argument style
	 * @param style
	 */
	void setArgumentStyle(ArgumentStyle::Enum style);
	
	/**
	 * @brief get the argument style
	 * @return
	 */
	ArgumentStyle::Enum argumentStyle();
	
	/**
	 * @brief define a boolean switch
	 * @param name the parameter name
	 * @param def the default value
	 */
	void addSwitch(QString name, bool def = false);
	
	/**
	 * @brief define an option that takes an additional argument
	 * @param name the parameter name
	 * @param def the default value
	 */
	void addOption(QString name, QVariant def = QVariant());
	
	/**
	 * @brief define a positional argument
	 * @param name the parameter name
	 * @param required wether this argument is required
	 * @param def the default value
	 */
	void addArgument(QString name, bool required = true, QVariant def = QVariant());
	
	/**
	 * @brief adds a flag to an existing parameter
	 * @param name the (existing) parameter name
	 * @param flag the flag character
	 * @see addSwitch addArgument addOption
	 * Note: any one parameter can only have one flag
	 */
	void addShortOpt(QString name, QChar flag);
	
	/**
	 * @brief adds documentation to a Parameter
	 * @param name the parameter name
	 * @param metavar a string to be displayed as placeholder for the value
	 * @param doc a QString containing the documentation
	 * Note: on positional arguments, metavar replaces the name as displayed.
	 *       on options , metavar replaces the value placeholder
	 */
	void addDocumentation(QString name, QString doc, QString metavar = QString());
	
	/**
	 * @brief generate a help message
	 * @param progName the program name to use in the help message
	 * @param helpIndent how much the parameter documentation should be indented
	 * @param flagsInUsage whether we should use flags instead of options in the usage
	 * @return a help message
	 */
	QString compileHelp(QString progName, int helpIndent = 22, bool flagsInUsage = true);
	
	/**
	 * @brief generate a short usage message
	 * @param progName the program name to use in the usage message
	 * @param useFlags whether we should use flags instead of options
	 * @return a usage message
	 */
	QString compileUsage(QString progName, bool useFlags = true);
	
	/**
	 * @brief parse
	 * @param argv a QStringList containing the program ARGV
	 * @return a QHash mapping argument names to their values
	 */
	QHash<QString, QVariant> parse(QStringList argv);
	
	/**
	 * @brief clear all definitions
	 */
	void clear();
	
	~Parser();
	
private:
	FlagStyle::Enum m_flagStyle;
	ArgumentStyle::Enum m_argStyle;

	enum OptionType
	{
		otSwitch,
		otOption
	};
	
	// Important: the common part MUST BE COMMON ON ALL THREE structs
	struct CommonDef {
		QString name;
		QString doc;
		QString metavar;
		QVariant def;
	};
	
	struct OptionDef {
		// common
		QString name;
		QString doc;
		QString metavar;
		QVariant def;
		// option
		OptionType type;
		QChar flag;
	};
	
	struct PositionalDef {
		// common
		QString name;
		QString doc;
		QString metavar;
		QVariant def;
		// positional
		bool required;
	};
	
	QHash<QString, OptionDef *> m_options;
	QHash<QChar, OptionDef *> m_flags;
	QHash<QString, CommonDef *> m_params;
	QList<PositionalDef *> m_positionals;
	QList<OptionDef *> m_optionList;
	
	void getPrefix(QString &opt, QString &flag);
};

}
}

#endif // CMDUTILS_H
