// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "SharedOptionsFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QSaveFile>

#include <utility>

namespace {

struct OptionsLine {
    QByteArray contents;
    QByteArray terminator;
    QString key;
    QByteArray value;
    bool isOption = false;
};

struct OptionsDocument {
    QList<OptionsLine> lines;
    QByteArray preferredTerminator = "\n";
};

OptionsDocument parse(const QByteArray& data)
{
    OptionsDocument document;
    qsizetype offset = 0;
    bool foundTerminator = false;

    while (offset < data.size()) {
        const auto newline = data.indexOf('\n', offset);
        const bool hasTerminator = newline >= 0;
        const auto end = hasTerminator ? newline : data.size();

        OptionsLine line;
        if (hasTerminator && end > offset && data.at(end - 1) == '\r') {
            line.contents = data.mid(offset, end - offset - 1);
            line.terminator = "\r\n";
        } else {
            line.contents = data.mid(offset, end - offset);
            line.terminator = hasTerminator ? QByteArray("\n") : QByteArray();
        }

        if (!foundTerminator && !line.terminator.isEmpty()) {
            document.preferredTerminator = line.terminator;
            foundTerminator = true;
        }

        const auto separator = line.contents.indexOf(':');
        if (separator > 0) {
            line.key = QString::fromUtf8(line.contents.constData(), separator);
            line.value = line.contents.mid(separator + 1);
            line.isOption = true;
        }
        document.lines.append(std::move(line));

        if (!hasTerminator) {
            break;
        }
        offset = newline + 1;
    }

    return document;
}

QByteArray serialize(const OptionsDocument& document)
{
    QByteArray result;
    for (const auto& line : document.lines) {
        result += line.contents;
        result += line.terminator;
    }
    return result;
}

QHash<QString, QByteArray> values(const OptionsDocument& document)
{
    QHash<QString, QByteArray> result;
    for (const auto& line : document.lines) {
        if (line.isOption) {
            result.insert(line.key, line.value);
        }
    }
    return result;
}

QSet<QString> keys(const OptionsDocument& document)
{
    QSet<QString> result;
    for (const auto& line : document.lines) {
        if (line.isOption) {
            result.insert(line.key);
        }
    }
    return result;
}

void replaceValues(OptionsDocument& document, const QHash<QString, QByteArray>& replacements, const QSet<QString>& excludedKeys)
{
    for (auto& line : document.lines) {
        if (!line.isOption || excludedKeys.contains(line.key)) {
            continue;
        }
        const auto replacement = replacements.constFind(line.key);
        if (replacement != replacements.cend()) {
            line.value = replacement.value();
            line.contents = line.key.toUtf8() + ':' + line.value;
        }
    }
}

void appendLine(OptionsDocument& document, OptionsLine line)
{
    if (!document.lines.isEmpty() && document.lines.constLast().terminator.isEmpty()) {
        document.lines.last().terminator = document.preferredTerminator;
    }
    line.terminator = document.preferredTerminator;
    document.lines.append(std::move(line));
}

OptionsDocument withoutExcludedOptions(const OptionsDocument& source, const QSet<QString>& excludedKeys)
{
    OptionsDocument result;
    result.preferredTerminator = source.preferredTerminator;
    for (const auto& line : source.lines) {
        if (!line.isOption || !excludedKeys.contains(line.key)) {
            result.lines.append(line);
        }
    }
    return result;
}

bool readFile(const QString& path, QByteArray& data, bool allowMissing, QString* error)
{
    QFile file(path);
    if (!file.exists() && allowMissing) {
        data.clear();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

bool writeFile(const QString& path, const QByteArray& data, QString* error)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error) {
            *error = QStringLiteral("Could not create the parent directory for %1").arg(path);
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    if (file.write(data) != data.size()) {
        if (error) {
            *error = file.errorString();
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

}

QByteArray SharedOptionsFile::overlay(const QByteArray& shared, const QByteArray& target, const QSet<QString>& excludedKeys)
{
    const auto sharedDocument = parse(shared);
    if (target.isEmpty()) {
        return serialize(withoutExcludedOptions(sharedDocument, excludedKeys));
    }

    auto targetDocument = parse(target);
    replaceValues(targetDocument, values(sharedDocument), excludedKeys);
    auto targetKeys = keys(targetDocument);
    for (const auto& line : sharedDocument.lines) {
        if (!line.isOption || excludedKeys.contains(line.key) || targetKeys.contains(line.key)) {
            continue;
        }
        appendLine(targetDocument, line);
        targetKeys.insert(line.key);
    }
    return serialize(targetDocument);
}

QByteArray SharedOptionsFile::mergeIntoShared(const QByteArray& shared, const QByteArray& local, const QSet<QString>& excludedKeys)
{
    const auto localDocument = parse(local);
    if (shared.isEmpty()) {
        return serialize(withoutExcludedOptions(localDocument, excludedKeys));
    }

    auto sharedDocument = parse(shared);
    replaceValues(sharedDocument, values(localDocument), excludedKeys);

    auto sharedKeys = keys(sharedDocument);
    for (const auto& line : localDocument.lines) {
        if (!line.isOption || excludedKeys.contains(line.key) || sharedKeys.contains(line.key)) {
            continue;
        }
        appendLine(sharedDocument, line);
    }

    return serialize(sharedDocument);
}

bool SharedOptionsFile::overlayFile(const QString& sharedPath, const QString& targetPath, const QSet<QString>& excludedKeys, QString* error)
{
    if (error) {
        error->clear();
    }
    QByteArray shared;
    QByteArray target;
    if (!readFile(sharedPath, shared, false, error) || !readFile(targetPath, target, true, error)) {
        return false;
    }
    return writeFile(targetPath, overlay(shared, target, excludedKeys), error);
}

bool SharedOptionsFile::updateSharedFile(const QString& localPath,
                                         const QString& sharedPath,
                                         const QSet<QString>& excludedKeys,
                                         QString* error)
{
    if (error) {
        error->clear();
    }
    QByteArray local;
    QByteArray shared;
    if (!readFile(localPath, local, false, error) || !readFile(sharedPath, shared, true, error)) {
        return false;
    }
    return writeFile(sharedPath, mergeIntoShared(shared, local, excludedKeys), error);
}
