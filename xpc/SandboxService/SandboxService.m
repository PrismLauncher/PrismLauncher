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

#import "SandboxService.h"
#import <AppKit/AppKit.h>
#import <CommonCrypto/CommonDigest.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#import <mach-o/fat.h>
#import <mach-o/loader.h>
#import <sys/xattr.h>

@interface SandboxService (Private)
/// Check whether the Mach-O object file data corresponding to the `fileHandle` contains an entry point; that is, if it has a `main`
/// function that will run when executed. `objectOffset` is the offset in the file where the Mach-O object file data starts for a particular
/// architecture.
///
/// If the data is invalid, there is assumed to be an entry point (i.e. a possibly dangerous file).
- (BOOL)entrypointExistsForFileHandle:(NSFileHandle*)fileHandle atOffset:(NSUInteger)objectOffset;
/// Check whether the Mach-O file at the given URL has an entry point (i.e. a `main` function that will run when executed).
/// If the file is not a Mach-O file, this function returns `YES`.
- (BOOL)entrypointExistsInExecutableFileAt:(NSURL*)url;
- (BOOL)shouldRemoveQuarantineOnFileAt:(NSURL*)url;
- (BOOL)removeQuarantineForFileAt:(NSURL*)fileURL;
- (BOOL)verifyJavaRuntimeAt:(NSURL*)url againstFileManifest:(NSDictionary*)files;
@end

@implementation SandboxService
- (void)removeQuarantineFromFileAt:(NSString*)path withReply:(void (^)(BOOL*, NSString*))reply
{
    BOOL result = NO;
    NSURL* url = [NSURL fileURLWithPath:path];

    // Copy the file to a temporary location outside the sandbox, so the sandboxed code can't interfere with the below operations (for
    // example, trying to set the executable bit).
    NSError* err = nil;
    NSURL* temporaryDirectory = [[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory
                                                                       inDomain:NSUserDomainMask
                                                              appropriateForURL:url
                                                                         create:YES
                                                                          error:&err];
    if (err) {
        NSLog(@"An error occurred while creating a temporary directory: %@", [err localizedDescription]);
        reply(&result, path);
        return;
    }
    NSURL* unquarantinedCopyURL = [temporaryDirectory URLByAppendingPathComponent:[url lastPathComponent]];
    [[NSFileManager defaultManager] copyItemAtURL:url toURL:unquarantinedCopyURL error:&err];
    if (err) {
        NSLog(@"An error occurred while copying the file to a temporary location: %@", [err localizedDescription]);
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    }

    if (![self shouldRemoveQuarantineOnFileAt:unquarantinedCopyURL]) {
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    }

    // Clear the executable bit on the file to prevent a malicious item from being allowed to execute in Terminal.
    NSDictionary<NSFileAttributeKey, id>* attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:unquarantinedCopyURL.path
                                                                                                   error:&err];
    if (err) {
        NSLog(@"Couldn't read the file permissions: %@", [err localizedDescription]);
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    }
    NSNumber* newPosixPerms = [NSNumber numberWithShort:[[attrs valueForKey:NSFilePosixPermissions] shortValue] & 0666];
    NSDictionary<NSFileAttributeKey, id>* newAttr = @{ NSFilePosixPermissions : newPosixPerms };
    [[NSFileManager defaultManager] setAttributes:newAttr ofItemAtPath:unquarantinedCopyURL.path error:&err];
    if (err) {
        NSLog(@"Couldn't remove executable bit: %@", [err localizedDescription]);
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    }

    // Now that it is safe to do so, remove quarantine.
    [self removeQuarantineForFileAt:unquarantinedCopyURL];

    // Put the file back where it originally was.
    [[NSFileManager defaultManager] replaceItemAtURL:url
                                       withItemAtURL:unquarantinedCopyURL
                                      backupItemName:nil
                                             options:NSFileManagerItemReplacementUsingNewMetadataOnly
                                    resultingItemURL:nil
                                               error:&err];
    if (err) {
        NSLog(@"Couldn't copy back: %@", [err localizedDescription]);
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    } else {
        result = YES;
        reply(&result, path);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
    }
}

- (void)removeQuarantineRecursivelyFromJavaInstallAt:(NSString*)path
                            downloadedFromManifestAt:(NSURL*)manifestURL
                                           withReply:(void (^)(BOOL*))reply
{
    __block BOOL result = NO;
    NSURL* directoryURL = [NSURL fileURLWithPath:path];

    if (![manifestURL.scheme isEqualToString:@"https"] || ![manifestURL.host isEqualToString:@"piston-meta.mojang.com"]) {
        NSLog(@"Invalid manifest URL: %@", manifestURL);
        reply(&result);
        return;
    }

    // Copy the directory to a temporary location outside the sandbox, so the sandboxed code can't interfere with the below operations.
    __block NSError* err = nil;
    NSURL* temporaryDirectory = [[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory
                                                                       inDomain:NSUserDomainMask
                                                              appropriateForURL:directoryURL
                                                                         create:YES
                                                                          error:&err];
    if (err) {
        NSLog(@"An error occurred while creating a temporary directory for %@: %@", directoryURL, [err localizedDescription]);
        reply(&result);
        return;
    }
    NSURL* unquarantinedCopyURL = [temporaryDirectory URLByAppendingPathComponent:[directoryURL lastPathComponent]];
    [[NSFileManager defaultManager] copyItemAtURL:directoryURL toURL:unquarantinedCopyURL error:&err];
    if (err) {
        NSLog(@"An error occurred while copying the directory to a temporary location: %@", [err localizedDescription]);
        reply(&result);
        [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
        return;
    }

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* downloadTask =
        [session dataTaskWithURL:manifestURL
               completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
                 if (error) {
                     NSLog(@"Failed to download manifest: %@", error.localizedDescription);
                     reply(&result);
                     [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
                     return;
                 }

                 NSError* jsonError = nil;
                 NSDictionary* manifest = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
                 if (jsonError) {
                     NSLog(@"Failed to parse JSON manifest: %@", [jsonError localizedDescription]);
                     reply(&result);
                     [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
                     return;
                 }

                 NSDictionary* files = manifest[@"files"];
                 result = [self verifyJavaRuntimeAt:temporaryDirectory againstFileManifest:files];

                 if (!result) {
                     reply(&result);
                     [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
                     return;
                 }

                 // Recursively remove quarantine from all files in the directory.
                 NSDirectoryEnumerator* enumerator = [[NSFileManager defaultManager] enumeratorAtURL:temporaryDirectory
                                                                          includingPropertiesForKeys:@[ NSURLIsRegularFileKey ]
                                                                                             options:0
                                                                                        errorHandler:nil];
                 for (NSURL* fileURL in enumerator) {
                     BOOL removed = [self removeQuarantineForFileAt:fileURL];
                     if (!removed) {
                         NSLog(@"Failed to remove quarantine from %@", fileURL.path);
                         result = NO;
                         reply(&result);
                         [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
                         return;
                     }
                 }

                 // Put the directory back where it originally was.
                 [[NSFileManager defaultManager] replaceItemAtURL:directoryURL
                                                    withItemAtURL:unquarantinedCopyURL
                                                   backupItemName:nil
                                                          options:NSFileManagerItemReplacementUsingNewMetadataOnly
                                                 resultingItemURL:nil
                                                            error:&err];
                 if (err) {
                     NSLog(@"Couldn't copy back: %@", [err localizedDescription]);
                     reply(&result);
                     [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
                     return;
                 }

                 reply(&result);
                 [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectory error:nil];
               }];

    [downloadTask resume];
}

- (void)applyDownloadQuarantineRecursivelyToJavaInstallAt:(NSString*)path withReply:(void (^)(BOOL*))reply
{
    BOOL result = YES;
    NSURL* directoryURL = [NSURL fileURLWithPath:path];
    NSDirectoryEnumerator* enumerator = [[NSFileManager defaultManager] enumeratorAtURL:directoryURL
                                                             includingPropertiesForKeys:@[ NSURLIsRegularFileKey ]
                                                                                options:0
                                                                           errorHandler:nil];
    NSDictionary* quarantineProperties = @{
        NSURLQuarantinePropertiesKey : @{
            (__bridge id)kLSQuarantineAgentNameKey : @"Prism Launcher",
            (__bridge id)kLSQuarantineTypeKey : (__bridge id)kLSQuarantineTypeOtherDownload,
        }
    };
    for (NSURL* fileURL in enumerator) {
        NSError* err = nil;
        [fileURL setResourceValue:@{} forKey:NSURLQuarantinePropertiesKey error:&err];
        if (err) {
            NSLog(@"Couldn't apply quarantine: %@", [err localizedDescription]);
            reply(&result);
            return;
        }
    }

    reply(&result);
}

- (void)retrieveUnsandboxedUserTemporaryDirectoryWithReply:(void (^)(NSString*))reply
{
    reply(NSTemporaryDirectory());
}

@end

@implementation SandboxService (Private)

- (BOOL)removeQuarantineForFileAt:(NSURL*)fileURL
{
    NSError* err = nil;
    [fileURL setResourceValue:nil forKey:NSURLQuarantinePropertiesKey error:&err];
    if (err) {
        NSLog(@"Couldn't remove quarantine: %@", [err localizedDescription]);
        return NO;
    }
    // Apple says to do the above, but it instead puts some kind of "quarantine removed" flag on some systems rather than just removing it.
    // There's a macOS bug (?) that causes Gatekeeper to still deny a dynamic library with such an attribute from loading. (FB15970881)
    // Removing the xattr directly works around this issue, though this isn't ideal since `com.apple.quarantine` is not technically API
    NSDictionary<NSURLResourceKey, id>* quarantineAfter = [fileURL resourceValuesForKeys:@[ NSURLQuarantinePropertiesKey ] error:nil];
    if (quarantineAfter[NSURLQuarantinePropertiesKey] != nil) {
        int removalResult = removexattr([fileURL fileSystemRepresentation], "com.apple.quarantine", XATTR_NOFOLLOW);
        if (removalResult != 0) {
            NSLog(@"Couldn't remove com.apple.quarantine on %@ directly", fileURL.path);
            // don't return here
            // failure here but not above probably means Apple changed how quarantine is stored, i.e. not com.apple.quarantine
            // hopefully by then (if that ever happens) the need for this workaround won't exist anymore anyway
        }
    }

    return YES;
}

- (BOOL)verifyJavaRuntimeAt:(NSURL*)url againstFileManifest:(NSDictionary*)files
{
    // Check if there are any files in the directory that are not listed in the manifest - these might be malicious.
    NSDirectoryEnumerator* enumerator =
        [[NSFileManager defaultManager] enumeratorAtURL:url
                             includingPropertiesForKeys:@[ NSURLIsRegularFileKey, NSURLIsSymbolicLinkKey, NSURLIsDirectoryKey ]
                                                options:0
                                           errorHandler:nil];
    NSMutableSet<NSString*>* verifiedFiles = [NSMutableSet setWithCapacity:[files count]];
    for (NSURL* fileURL in enumerator) {
        NSString* relativePath =
            [[[fileURL URLByStandardizingPath] path] substringFromIndex:[[url URLByStandardizingPath] path].length + 1];
        NSDictionary* fileInfo = files[relativePath];
        if (!fileInfo) {
            NSLog(@"Java runtime not verified due to extraneous file not listed in manifest: %@ (%@)", fileURL.path, relativePath);
            return NO;
        }

        if ([fileInfo[@"type"] isEqualToString:@"file"]) {
            NSNumber* isRegularFile;
            if (![fileURL getResourceValue:&isRegularFile forKey:NSURLIsRegularFileKey error:nil] || ![isRegularFile boolValue]) {
                NSLog(@"Java runtime not verified as %@ (%@) is not a regular file", fileURL.path, relativePath);
                return NO;
            }

            NSString* expectedChecksum = fileInfo[@"downloads"][@"raw"][@"sha1"];
            NSData* fileData = [NSData dataWithContentsOfURL:fileURL];
            if (!fileData) {
                NSLog(@"Java runtime not verified due to failure to read file: %@ (%@)", fileURL.path, relativePath);
                return NO;
            }
            if ([fileData length] != [fileInfo[@"downloads"][@"raw"][@"size"] unsignedLongLongValue]) {
                NSLog(@"Java runtime not verified due to size mismatch for file: %@ (%@)", fileURL.path, relativePath);
                return NO;
            }
            unsigned char actualChecksumData[CC_SHA1_DIGEST_LENGTH];
            CC_SHA1([fileData bytes], [fileData length], actualChecksumData);
            NSMutableString* actualChecksum = [NSMutableString stringWithCapacity:CC_SHA1_DIGEST_LENGTH * 2];
            for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++) {
                [actualChecksum appendFormat:@"%02x", actualChecksumData[i]];
            }

            if (![expectedChecksum isEqualToString:actualChecksum]) {
                NSLog(@"Java runtime not verified due to checksum mismatch for file: %@ (%@)", fileURL.path, relativePath);
                return NO;
            }
        } else if ([fileInfo[@"type"] isEqualToString:@"link"]) {
            NSNumber* isSymbolicLink;
            if (![fileURL getResourceValue:&isSymbolicLink forKey:NSURLIsSymbolicLinkKey error:nil] || ![isSymbolicLink boolValue]) {
                NSLog(@"Java runtime not verified as %@ (%@) is not a symbolic link", fileURL.path, relativePath);
                return NO;
            }

            NSURL* expectedDestination =
                [[[fileURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:fileInfo[@"target"]] URLByStandardizingPath];
            NSURL* actualDestination = [fileURL URLByResolvingSymlinksInPath];
            if (![expectedDestination isEqual:actualDestination]) {
                NSLog(@"Symbolic link mismatch for file: %@ (%@) (expected %@, got %@)", fileURL.path, relativePath, expectedDestination,
                      actualDestination);
                return NO;
            }
        } else if ([fileInfo[@"type"] isEqualToString:@"directory"]) {
            NSNumber* isDirectory;
            if (![fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil] || ![isDirectory boolValue]) {
                NSLog(@"Java runtime not verified as %@ (%@) is not a directory", fileURL.path, relativePath);
                return NO;
            }
        } else {
            NSLog(@"Java runtime not verified as %@ (%@) has an unknown type", fileURL.path, relativePath);
            return NO;
        }

        [verifiedFiles addObject:relativePath];
    }

    for (NSString* relativePath in files) {
        if (![verifiedFiles containsObject:relativePath]) {
            NSLog(@"Java runtime not verified due to missing file %@", relativePath);
            return NO;
        }
    }

    return YES;
}

- (BOOL)entrypointExistsForFileHandle:(NSFileHandle *)fileHandle atOffset:(NSUInteger)objectOffset
{
    if (![fileHandle seekToOffset:objectOffset error:nil]) {
        return YES;
    }
    NSData* headerData = [fileHandle readDataOfLength:sizeof(struct mach_header)];
    if (!headerData || [headerData length] < sizeof(struct mach_header)) {
        return YES;
    }
    const struct mach_header* header32 = (const struct mach_header*)[headerData bytes];

    uint32_t ncmds = header32->ncmds;
    NSUInteger cmdOffset;
    bool endiannessReversed;

    if (header32->magic == MH_MAGIC_64 || header32->magic == MH_CIGAM_64) {
        cmdOffset = sizeof(struct mach_header_64);
        endiannessReversed = header32->magic == MH_CIGAM_64;
    } else if (header32->magic == MH_MAGIC || header32->magic == MH_CIGAM) {
        cmdOffset = sizeof(struct mach_header);
        endiannessReversed = header32->magic == MH_CIGAM;
    } else {
        // don't know what kind of binary this is, just assume this has an entry point (and thus deny quarantine removal)
        return YES;
    }

    if (endiannessReversed) {
        ncmds = CFSwapInt32(ncmds);
    }

    for (uint32_t i = 0; i < ncmds; i++) {
        if (![fileHandle seekToOffset:objectOffset + cmdOffset error:nil]) {
            return YES;
        }
        NSData* cmdData = [fileHandle readDataOfLength:sizeof(struct load_command)];
        if (!cmdData || [cmdData length] < sizeof(struct load_command)) {
            return YES;
        }

        const struct load_command* cmd = (const struct load_command*)[cmdData bytes];

        uint32_t cmdType = endiannessReversed ? CFSwapInt32(cmd->cmd) : cmd->cmd;
        // found an entrypoint
        if (cmdType == LC_MAIN || cmdType == LC_UNIXTHREAD) {
            return YES;
        }

        uint32_t cmdSize = endiannessReversed ? CFSwapInt32(cmd->cmdsize) : cmd->cmdsize;
        cmdOffset += cmdSize;
    }

    return NO;
}

- (BOOL)entrypointExistsInExecutableFileAt:(NSURL *)url
{
    // read and interpret the Mach-O header and file data directly
    // note: generally there's a command line tool you can use to check this, e.g. `/usr/bin/otool`, but that requires a separate process,
    // the output on that is not guaranteed to be stable, and it's unknown if it's on the system by default for all supported macOS versions
    // luckily, there's public API (mach-o/*.h) to help us parse the format and do this ourselves, though it's quite messy
    NSFileHandle* fileHandle = [NSFileHandle fileHandleForReadingFromURL:url error:nil];
    if (fileHandle == nil) {
        return YES;
    }
    NSData* fatHeaderData = [fileHandle readDataOfLength:sizeof(struct fat_header)];
    if (!fatHeaderData || [fatHeaderData length] < sizeof(struct fat_header)) {
        return YES;
    }

    const struct fat_header* header = (const struct fat_header*)[fatHeaderData bytes];
    NSUInteger archOffset = sizeof(struct fat_header);

    size_t archSize;
    // anything declared in mach-o/fat.h is always stored on disk in big-endian order
    uint32_t numArch = CFSwapInt32BigToHost(header->nfat_arch);

    if (header->magic == FAT_MAGIC || header->magic == FAT_CIGAM) {
        archSize = sizeof(struct fat_arch);
    } else if (header->magic == FAT_MAGIC_64 || header->magic == FAT_CIGAM_64) {
        archSize = sizeof(struct fat_arch_64);
    } else if (header->magic == MH_MAGIC || header->magic == MH_CIGAM || header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
        // not a "fat" binary, just check the single architecture that is there
        return [self entrypointExistsForFileHandle:fileHandle atOffset:0];
    } else {
        // no idea what this file is - just say there's an entrypoint to be safe
        return YES;
    }

    if (![fileHandle seekToOffset:archOffset error:nil]) {
        return YES;
    }
    NSData* archData = [fileHandle readDataOfLength:archSize * numArch];
    if (!archData || [archData length] < archSize * numArch) {
        return YES;
    }
    for (int i = 0; i < numArch; i++) {
        const struct fat_arch* currentArch = (const struct fat_arch*)[archData bytes] + i;
        uint32_t offset = CFSwapInt32BigToHost(currentArch->offset);
        if ([self entrypointExistsForFileHandle:fileHandle atOffset:offset]) {
            return YES;
        }
    }

    return NO;
}

- (BOOL)shouldRemoveQuarantineOnFileAt:(NSURL *)url
{
    NSDictionary<NSURLResourceKey, id>* resourceValues = [url resourceValuesForKeys:@[
        NSURLIsRegularFileKey, NSURLIsApplicationKey, NSURLIsPackageKey, NSURLQuarantinePropertiesKey, NSURLContentTypeKey
    ]
                                                                              error:nil];
    if (resourceValues == nil) {
        return NO;
    }

    // Avoid unquarantining directories (such as bundles, which can include applications).
    if (resourceValues[NSURLIsRegularFileKey] == nil || ![resourceValues[NSURLIsRegularFileKey] boolValue]) {
        return NO;
    }

    // Pretty sure the regular file check should handle this, but just in case:
    if (resourceValues[NSURLIsApplicationKey] == nil || [resourceValues[NSURLIsApplicationKey] boolValue]) {
        return NO;
    }
    if (resourceValues[NSURLIsPackageKey] == nil || [resourceValues[NSURLIsPackageKey] boolValue]) {
        return NO;
    }

    // Ignore the file if it is not quarantined.
    if (resourceValues[NSURLQuarantinePropertiesKey] == nil ||
        resourceValues[NSURLQuarantinePropertiesKey][(__bridge id)kLSQuarantineTypeKey] == nil) {
        return NO;
    }

    // Check if the Mach-O binary file has a `main` method (would it execute something if it were run itself?).
    // It might be unsafe in that case, and is uncommon for dynamic libraries.
    // (if this isn't a Mach-O file, this returns YES, and thus we don't remove quarantine - there's no valid reason to remove quarantine
    // from a non-Mach-O file)
    // NOTE: this check might be too strict - should this be kept or relaxed? It seems to be ok for the vanilla game at least
    if ([self entrypointExistsInExecutableFileAt:url]) {
        return NO;
    }

    // If the "Open With" attribute on a file has been changed, that could potentially be dangerous. Only unquarantine a file if this
    // attribute is not set to a non-default value. Note that sandboxed processes can't choose to open a file in an app other than the
    // default app for that file, an app that declares it can open that file type, or certain "safe" apps (like TextEdit).
    if (@available(macOS 12.0, *)) {
        UTType* fileType = resourceValues[NSURLContentTypeKey];
        if (fileType == nil || [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:url] !=
                                   [[NSWorkspace sharedWorkspace] URLForApplicationToOpenContentType:fileType]) {
            return NO;
        }
    }

    NSSet<NSString*>* allowedExtensions = [[NSSet alloc] initWithArray:@[ @"", @"dylib", @"tmp", @"jnilib", @"so" ]];
    return [allowedExtensions containsObject:url.pathExtension];
}

@end
