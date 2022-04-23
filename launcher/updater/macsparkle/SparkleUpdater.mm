// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Kenneth Chew <kenneth.c0@protonmail.com>
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

#include "SparkleUpdater.h"

#include "Application.h"

#include <Cocoa/Cocoa.h>
#include <Sparkle/Sparkle.h>

@interface UpdaterObserver : NSObject

@property(nonatomic, readonly) SPUUpdater* updater;

/// A callback to run when the state of `canCheckForUpdates` for the `updater` changes.
@property(nonatomic, copy) void (^callback) (bool);

- (id)initWithUpdater:(SPUUpdater*)updater;

@end

@implementation UpdaterObserver

- (id)initWithUpdater:(SPUUpdater*)updater
{
    self = [super init];
    _updater = updater;
    [self addObserver:self forKeyPath:@"updater.canCheckForUpdates" options:NSKeyValueObservingOptionNew context:nil];

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context
{
    if ([keyPath isEqualToString:@"updater.canCheckForUpdates"])
    {
        bool canCheck = [change[NSKeyValueChangeNewKey] boolValue];
        self.callback(canCheck);
    }
}

@end


@interface UpdaterDelegate : NSObject <SPUUpdaterDelegate>

@property(nonatomic, copy) NSSet<NSString *> *allowedChannels;

@end

@implementation UpdaterDelegate

- (NSSet<NSString *> *)allowedChannelsForUpdater:(SPUUpdater *)updater
{
    return _allowedChannels;
}

@end


class SparkleUpdater::Private
{
public:
    SPUStandardUpdaterController *updaterController;
    UpdaterObserver *updaterObserver;
    UpdaterDelegate *updaterDelegate;
    NSAutoreleasePool *autoReleasePool;
};

SparkleUpdater::SparkleUpdater()
{
    priv = new SparkleUpdater::Private();

    // Enable Cocoa's memory management.
    NSApplicationLoad();
    priv->autoReleasePool = [[NSAutoreleasePool alloc] init];

    // Delegate is used for setting/getting allowed update channels.
    priv->updaterDelegate = [[UpdaterDelegate alloc] init];

    // Controller is the interface for actually doing the updates.
    priv->updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:true
                               updaterDelegate:priv->updaterDelegate
                               userDriverDelegate:nil];

    priv->updaterObserver = [[UpdaterObserver alloc] initWithUpdater:priv->updaterController.updater];
    // Use KVO to run a callback that emits a Qt signal when `canCheckForUpdates` changes, so the UI can respond accordingly.
    priv->updaterObserver.callback = ^(bool canCheck) {
        emit canCheckForUpdatesChanged(canCheck);
    };

    loadChannelsFromSettings();
}

SparkleUpdater::~SparkleUpdater()
{
    [priv->updaterObserver removeObserver:priv->updaterObserver forKeyPath:@"updater.canCheckForUpdates"];

    [priv->updaterController release];
    [priv->updaterObserver release];
    [priv->updaterDelegate release];
    [priv->autoReleasePool release];
    delete priv;
}

void SparkleUpdater::checkForUpdates()
{
    [priv->updaterController checkForUpdates:nil];
}

bool SparkleUpdater::getAutomaticallyChecksForUpdates()
{
    return priv->updaterController.updater.automaticallyChecksForUpdates;
}

double SparkleUpdater::getUpdateCheckInterval()
{
    return priv->updaterController.updater.updateCheckInterval;
}

QSet<QString> SparkleUpdater::getAllowedChannels()
{
    // Convert NSSet<NSString> -> QSet<QString>
    __block QSet<QString> channels;
    [priv->updaterDelegate.allowedChannels enumerateObjectsUsingBlock:^(NSString *channel, BOOL *stop)
    {
        channels.insert(QString::fromNSString(channel));
    }];
    return channels;
}

void SparkleUpdater::setAutomaticallyChecksForUpdates(bool check)
{
    priv->updaterController.updater.automaticallyChecksForUpdates = check ? YES : NO; // make clang-tidy happy
}

void SparkleUpdater::setUpdateCheckInterval(double seconds)
{
    priv->updaterController.updater.updateCheckInterval = seconds;
}

void SparkleUpdater::clearAllowedChannels()
{
    priv->updaterDelegate.allowedChannels = [NSSet set];
    APPLICATION->settings()->set("UpdateChannel", "");
}

void SparkleUpdater::setAllowedChannel(const QString &channel)
{
    if (channel.isEmpty())
    {
        clearAllowedChannels();
        return;
    }

    NSSet<NSString *> *nsChannels = [NSSet setWithObject:channel.toNSString()];
    priv->updaterDelegate.allowedChannels = nsChannels;
    APPLICATION->settings()->set("UpdateChannel", channel);
}

void SparkleUpdater::setAllowedChannels(const QSet<QString> &channels)
{
    if (channels.isEmpty())
    {
        clearAllowedChannels();
        return;
    }

    QString channelsConfig = "";
    // Convert QSet<QString> -> NSSet<NSString>
    NSMutableSet<NSString *> *nsChannels = [NSMutableSet setWithCapacity:channels.count()];
    foreach (const QString channel, channels)
    {
        [nsChannels addObject:channel.toNSString()];
        channelsConfig += channel + " ";
    }

    priv->updaterDelegate.allowedChannels = nsChannels;
    APPLICATION->settings()->set("UpdateChannel", channelsConfig.trimmed());
}

void SparkleUpdater::loadChannelsFromSettings()
{
    QStringList channelList = APPLICATION->settings()->get("UpdateChannel").toString().split(" ");
    auto channels = QSet<QString>::fromList(channelList);
    setAllowedChannels(channels);
}
