#pragma once

#include <QWidget>

namespace GuiUtil
{
void uploadPaste(const QString &text, QWidget *parentWidget);
void setClipboardText(const QString &text);
QStringList BrowseForMods(QString context, QString caption, QString filter, QWidget *parentWidget);
}
