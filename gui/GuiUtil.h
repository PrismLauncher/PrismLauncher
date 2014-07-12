#pragma once

#include <QWidget>

namespace GuiUtil
{
void uploadPaste(const QString &text, QWidget *parentWidget);
void setClipboardText(const QString &text);
}
