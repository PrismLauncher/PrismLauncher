#pragma once

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QPalette>
#include <QWidget>
class QStyle;

#include <optional>

#include "Result.h"

namespace GuiUtil {
Result<std::optional<QString>> uploadPaste(const QString& name, const QFileInfo& filePath, QWidget* parentWidget);
Result<std::optional<QString>> uploadPaste(const QString& name, const QString& data, QWidget* parentWidget);
void setClipboardText(QString text);
QStringList browseForFiles(const QString& context,
                           const QString& caption,
                           const QString& filter,
                           const QString& defaultPath,
                           QWidget* parentWidget);
QString browseForFile(const QString& context,
                      const QString& caption,
                      const QString& filter,
                      const QString& defaultPath,
                      QWidget* parentWidget);

QColor selectedItemFg(const QPalette& palette, const QStyle* style = QApplication::style());
}  // namespace GuiUtil
