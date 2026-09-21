// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include <QByteArray>
#include <QSet>
#include <QString>

class SharedOptionsFile {
   public:
    static QByteArray overlay(const QByteArray& shared, const QByteArray& target, const QSet<QString>& excludedKeys = {});

    static QByteArray mergeIntoShared(const QByteArray& shared, const QByteArray& local, const QSet<QString>& excludedKeys = {});

    static bool overlayFile(const QString& sharedPath,
                            const QString& targetPath,
                            const QSet<QString>& excludedKeys = {},
                            QString* error = nullptr);

    static bool updateSharedFile(const QString& localPath,
                                 const QString& sharedPath,
                                 const QSet<QString>& excludedKeys = {},
                                 QString* error = nullptr);
};
