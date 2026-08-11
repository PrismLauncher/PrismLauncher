// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ThemeManager.h"

#include <AppKit/AppKit.h>
#include <QApplication>

void ThemeManager::setTitlebarColorOnMac(WId windowId, QColor color) {
    if (windowId == 0) {
        return;
    }

    NSView* view = (NSView*)windowId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:color.alphaF()];
}

void ThemeManager::setTitlebarColorOfAllWindowsOnMac(QColor color) {
    NSArray<NSWindow*>* windows = [NSApp windows];
    for (NSWindow* window : windows) {
        setTitlebarColorOnMac((WId)window.contentView, color);
    }

    // We want to change the titlebar color of newly opened windows as well.
    // There's no notification for when a new window is opened, but we can set the color when a window switches
    // from occluded to visible, which also fires on open.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    stopSettingNewWindowColorsOnMac();
    m_windowTitlebarObserver = [center addObserverForName:NSWindowDidChangeOcclusionStateNotification
                                                   object:nil
                                                    queue:[NSOperationQueue mainQueue]
                                               usingBlock:^(NSNotification* notification) {
                                                 NSWindow* window = notification.object;
                                                 setTitlebarColorOnMac((WId)window.contentView, qApp->palette().window().color());
                                               }];
}

void ThemeManager::stopSettingNewWindowColorsOnMac() {
    if (m_windowTitlebarObserver) {
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center removeObserver:m_windowTitlebarObserver];
        m_windowTitlebarObserver = nil;
    }
}
