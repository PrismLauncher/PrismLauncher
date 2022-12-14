#pragma once

#include <QWidget>

namespace GuiUtil
{
QString uploadPaste(const QString &name, const QString &text, QWidget *parentWidget);
void setClipboardText(const QString &text);
QStringList BrowseForFiles(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget);
QString BrowseForFile(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget);
}
