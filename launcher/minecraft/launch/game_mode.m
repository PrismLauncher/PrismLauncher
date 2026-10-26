// SPDX-License-Identifier: GPL-3.0-only

#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <mach/mach.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <unistd.h>

typedef const void* LSASNRef;
typedef int LSSessionID;

static void* launchServices(void)
{
    return dlopen(
        "/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/LaunchServices",
        RTLD_NOW
    );
}

static LSASNRef copySelfASN(void* ls)
{
    LSASNRef (*copyForToken)(LSSessionID, audit_token_t) = dlsym(ls, "_LSCopyLSASNForAuditToken");
    if (!copyForToken)
        return NULL;
    audit_token_t token = {0};
    mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
    if (task_info(mach_task_self(), TASK_AUDIT_TOKEN, (task_info_t)&token, &count) != KERN_SUCCESS)
        return NULL;
    return copyForToken(-2, token);
}

__attribute__((visibility("default"), used))
int Agent_OnLoad(void* vm, char* options, void* reserved)
{
    (void)vm;
    (void)reserved;
    @autoreleasepool {
        void* ls = launchServices();
        CFDictionaryRef (*checkIn)(LSSessionID, CFDictionaryRef) = dlsym(ls, "_LSApplicationCheckIn");
        LSASNRef (*fromU64)(CFAllocatorRef, uint64_t) = dlsym(ls, "_LSASNCreateWithUInt64");
        CFStringRef* parentKeyPtr = dlsym(ls, "_kLSParentASNKey");
        CFStringRef parentKey = parentKeyPtr ? *parentKeyPtr : CFSTR("LSParentASN");

        uint64_t raw = options ? strtoull(options, NULL, 10) : 0;
        if (checkIn && fromU64 && raw != 0) {
            LSASNRef parentASN = fromU64(kCFAllocatorDefault, raw);
            if (parentASN) {
                checkIn(-2, (__bridge CFDictionaryRef)@{
                    (__bridge NSString*)parentKey: (__bridge id)parentASN,
                    @"ApplicationType": @"UIElement",
                    @"LSUIElement": @YES,
                });
                CFRelease(parentASN);
            }
        }
    }
    return 0;
}

static int runBeacon(pid_t parentPid)
{
    @autoreleasepool {
        int kq = kqueue();
        if (kq >= 0) {
            struct kevent kev;
            EV_SET(&kev, parentPid, EVFILT_PROC, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_EXIT, 0, NULL);
            if (kevent(kq, &kev, 1, NULL, 0, NULL) < 0) {
                close(kq);
                kq = -1;
            }
        }

        void* ls = launchServices();
        CFDictionaryRef (*checkIn)(LSSessionID, CFDictionaryRef) = dlsym(ls, "_LSApplicationCheckIn");
        OSStatus (*setItem)(LSSessionID, LSASNRef, CFStringRef, CFTypeRef, CFDictionaryRef*) =
            dlsym(ls, "_LSSetApplicationInformationItem");
        uint64_t (*toU64)(LSASNRef) = dlsym(ls, "_LSASNToUInt64");

        uint64_t raw = 0;
        if (checkIn && setItem && toU64) {
            checkIn(-2, (__bridge CFDictionaryRef)@{
                (__bridge NSString*)kCFBundleIdentifierKey: @"com.mojang.minecraftlauncher",
                @"ApplicationType": @"BackgroundOnly",
                @"LSBackgroundOnly": @YES,
                @"LSUIElement": @YES,
            });
            LSASNRef asn = copySelfASN(ls);
            if (asn) {
                setItem(-2, asn, kCFBundleIdentifierKey, CFSTR("com.mojang.minecraftlauncher"), NULL);
                raw = toU64(asn);
                CFRelease(asn);
            }
        }

        (void)write(STDOUT_FILENO, &raw, sizeof(raw));
        close(STDOUT_FILENO);

        if (kq >= 0) {
            struct kevent out;
            struct timespec timeout = { .tv_sec = 5, .tv_nsec = 0 };
            kevent(kq, NULL, 0, &out, 1, &timeout);
            close(kq);
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 3 && strcmp(argv[1], "--beacon") == 0)
        return runBeacon((pid_t)atoi(argv[2]));
    if (argc < 2)
        return 64;

    if (@available(macOS 14.0, *)) {
        int pipefd[2];
        if (pipe(pipefd) == 0) {
            char ppid[32];
            snprintf(ppid, sizeof(ppid), "%d", (int)getpid());
            char* beaconArgv[] = { argv[0], "--beacon", ppid, NULL };

            posix_spawn_file_actions_t actions;
            posix_spawn_file_actions_init(&actions);
            posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
            posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
            posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

            posix_spawnattr_t attr;
            posix_spawnattr_init(&attr);
            posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT);

            pid_t beaconPid = 0;
            extern char** environ;
            int rc = posix_spawn(&beaconPid, argv[0], &actions, &attr, beaconArgv, environ);
            posix_spawnattr_destroy(&attr);
            posix_spawn_file_actions_destroy(&actions);
            close(pipefd[1]);

            uint64_t rawASN = 0;
            if (rc == 0)
                (void)read(pipefd[0], &rawASN, sizeof(rawASN));
            close(pipefd[0]);

            if (rawASN != 0) {
                @autoreleasepool {
                    NSString* selfPath = [NSString stringWithUTF8String:argv[0]].stringByResolvingSymlinksInPath;
                    NSString* agentArg = [NSString stringWithFormat:@"-agentpath:%@=%llu", selfPath, (unsigned long long)rawASN];
                    char** javaArgv = calloc((size_t)argc + 1, sizeof(char*));
                    if (javaArgv) {
                        javaArgv[0] = argv[1];
                        javaArgv[1] = strdup(agentArg.UTF8String);
                        for (int i = 2; i < argc; i++)
                            javaArgv[i] = argv[i];
                        javaArgv[argc] = NULL;
                        execv(argv[1], javaArgv);
                    }
                }
            }
        }
    }

    execv(argv[1], argv + 1);
    int error = errno;
    perror(argv[1]);
    return error == ENOENT ? 127 : 126;
}
