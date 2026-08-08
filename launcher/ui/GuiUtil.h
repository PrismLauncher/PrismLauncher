#pragma once

#include <QFileInfo>
#include <QWidget>
#include <optional>

namespace GuiUtil {
std::optional<QString> uploadPaste(const QString& name, const QFileInfo& filePath, QWidget* parentWidget);
std::optional<QString> uploadPaste(const QString& name, const QString& data, QWidget* parentWidget);
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
}  // namespace GuiUtil
