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

void ThemeManager::setTitlebarColorOnMac(WId windowId, QColor color)
{
    if (windowId == 0) {
        return;
    }

    NSView* view = (NSView*)windowId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:color.alphaF()];

    // Unfortunately there seems to be no easy way to set the titlebar text color.
    // The closest we can do without dubious hacks is set the dark/light mode state based on the brightness of the
    // background color, which should at least make the text readable even if we can't use the theme's text color.
    // It's a good idea to set this anyway since it also affects some other UI elements like text shadows (PrismLauncher#3825).
    if (color.lightnessF() < 0.5) {
        window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    } else {
        window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    }
}

void ThemeManager::setTitlebarColorOfAllWindowsOnMac(QColor color)
{
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
                                                   setTitlebarColorOnMac((WId)window.contentView, color);
                                               }];
}

void ThemeManager::stopSettingNewWindowColorsOnMac()
{
    if (m_windowTitlebarObserver) {
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center removeObserver:m_windowTitlebarObserver];
        m_windowTitlebarObserver = nil;
    }
}

// Identifier to find our injected vibrancy views
static NSString* const kVibrancyViewIdentifier = @"PrismLauncherVibrancy";

static void addVibrancyToWindow(NSWindow* window)
{
    NSView* contentView = window.contentView;
    if (!contentView || !contentView.superview)
        return;

    NSView* themeFrame = contentView.superview;

    // Check if we already added a vibrancy view
    for (NSView* subview in themeFrame.subviews) {
        if ([subview.identifier isEqualToString:kVibrancyViewIdentifier])
            return;
    }

    // Create NSVisualEffectView with dark under-window material
    NSVisualEffectView* vibrancyView = [[NSVisualEffectView alloc] initWithFrame:themeFrame.bounds];
    vibrancyView.identifier = kVibrancyViewIdentifier;
    vibrancyView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    vibrancyView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    vibrancyView.material = NSVisualEffectMaterialHUDWindow;
    vibrancyView.state = NSVisualEffectStateActive;
    vibrancyView.wantsLayer = YES;

    // Insert into the themeFrame BEHIND the contentView so Qt widgets stay visible
    [themeFrame addSubview:vibrancyView positioned:NSWindowBelow relativeTo:contentView];

    // Titlebar styling
    window.titlebarAppearsTransparent = YES;
    window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
}

static void removeVibrancyFromWindow(NSWindow* window)
{
    NSView* contentView = window.contentView;
    if (!contentView || !contentView.superview)
        return;

    NSView* themeFrame = contentView.superview;

    NSMutableArray<NSView*>* toRemove = [NSMutableArray array];
    for (NSView* subview in themeFrame.subviews) {
        if ([subview.identifier isEqualToString:kVibrancyViewIdentifier]) {
            [toRemove addObject:subview];
        }
    }
    for (NSView* view in toRemove) {
        [view removeFromSuperview];
    }

    // Restore window defaults
    window.titlebarAppearsTransparent = NO;
    window.appearance = nil;
}

void ThemeManager::enableVibrancyOnAllWindows(bool enable)
{
    m_vibrancyEnabled = enable;

    NSArray<NSWindow*>* windows = [NSApp windows];
    for (NSWindow* window in windows) {
        if (enable) {
            addVibrancyToWindow(window);
        } else {
            removeVibrancyFromWindow(window);
        }
    }

    // Handle newly opened windows
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    if (m_vibrancyWindowObserver) {
        [center removeObserver:m_vibrancyWindowObserver];
        m_vibrancyWindowObserver = nil;
    }

    if (enable) {
        m_vibrancyWindowObserver = [center addObserverForName:NSWindowDidChangeOcclusionStateNotification
                                                        object:nil
                                                         queue:[NSOperationQueue mainQueue]
                                                    usingBlock:^(NSNotification* notification) {
                                                        if (m_vibrancyEnabled) {
                                                            NSWindow* window = notification.object;
                                                            addVibrancyToWindow(window);
                                                        }
                                                    }];
    }
}
