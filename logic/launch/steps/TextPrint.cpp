#include "TextPrint.h"

TextPrint::TextPrint(LaunchTask * parent, const QStringList &lines, MessageLevel::Enum level) : LaunchStep(parent)
{
	m_lines = lines;
	m_level = level;
}
TextPrint::TextPrint(LaunchTask *parent, const QString &line, MessageLevel::Enum level) : LaunchStep(parent)
{
	m_lines.append(line);
	m_level = level;
}

void TextPrint::executeTask()
{
	emit logLines(m_lines, m_level);
	emitSucceeded();
}

bool TextPrint::canAbort() const
{
	return true;
}

bool TextPrint::abort()
{
	emitFailed("Aborted.");
	return true;
}
