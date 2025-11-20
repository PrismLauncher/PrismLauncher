// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
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

#import <Foundation/Foundation.h>

@protocol SandboxServiceProtocol
/// Ask the service to remove quarantine from the file at `path`.
///
/// Some metadata of the file may be modified to prevent a sandbox escape. For example, the executable bit on a file may be removed.
///
/// \param path: The path of the file to remove quarantine for.
/// \param reply: A boolean indicating whether quarantine was removed, and the path of the unquarantined item. Note that `NO` doesn't necessarily mean the file is currently quarantined.
- (void)removeQuarantineFromFileAt:(NSString *)path withReply:(void (^)(BOOL *, NSString *))reply;
/// Ask the service to remove quarantine from the directory at `path`. The directory is intended to be a Java runtime downloaded from the
/// given manifest. The manifest must come from Mojang (piston-meta.mojang.com) and all files inside the directory must match the checksums.
///
/// \param path The path of a directory containing a Java runtime to remove quarantine for.
/// \param manifestURL A URL to a Mojang manifest that the Java runtime was downloaded from.
/// \param reply A boolean indicating whether quarantine was removed.
- (void)removeQuarantineRecursivelyFromJavaInstallAt:(NSString*)path
                            downloadedFromManifestAt:(NSURL*)manifestURL
                                           withReply:(void (^)(BOOL *))reply;
/// Apply a quarantine to all files at the provided `path` that indicates that the files were downloaded from the Internet. Unlike the
/// typical sandbox quarantine applied by default, a download quarantine allows executables to run if they are able to get past
/// Gatekeeper.
///
/// \param path The path of a directory containing files to apply quarantine to.
/// \param reply A boolean indicating whether quarantine was applied.
- (void)applyDownloadQuarantineRecursivelyToJavaInstallAt:(NSString*)path
                                               withReply:(void (^)(BOOL *))reply;
/// Get the value of NSTemporaryDirectory() for a nonsandboxed process.
///
/// \param reply The path of the temporary directory.
- (void)retrieveUnsandboxedUserTemporaryDirectoryWithReply:(void (^)(NSString *))reply;
@end
