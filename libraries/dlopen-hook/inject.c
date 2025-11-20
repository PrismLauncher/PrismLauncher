#include <dlfcn.h>
#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslimits.h>
#include <sys/un.h>

// See https://github.com/apple-opensource/dyld/blob/e3f88907bebb8421f50f0943595f6874de70ebe0/include/mach-o/dyld.h
#define DYLD_INTERPOSE(_replacment, _replacee)                                                                            \
    __attribute__((used)) static struct {                                                                                 \
        const void* replacment;                                                                                           \
        const void* replacee;                                                                                             \
    } _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, \
                                                                                (const void*)(unsigned long)&_replacee };

void* dlopen_new(const char* path, int mode)
{
    // skip for a few common system library locations, to avoid the overhead of sending the path over IPC
    const char* system = "/System";
    const char* library = "/Library";
    const char* usr = "/usr";
    if (path == NULL || strncmp(system, path, strlen(system)) == 0 || strncmp(library, path, strlen(library)) == 0 ||
        strncmp(usr, path, strlen(usr)) == 0) {
        return dlopen(path, mode);
    }

    const char* socket_input = getenv("PRISM_XPC_MIDDLEMAN_SOCKET");
    if (socket_input == NULL || strlen(socket_input) == 0 || socket_input[0] == '-') {
        printf("[PRISM SANDBOX WORKAROUND] PRISM_XPC_MIDDLEMAN_SOCKET not set, attempting to load library anyway\n");
        return dlopen(path, mode);
    }
    int sock = atoi(socket_input);
    if (sock <= 0) {
        printf("[PRISM SANDBOX WORKAROUND] PRISM_XPC_MIDDLEMAN_SOCKET invalid, attempting to load library anyway\n");
        return dlopen(path, mode);
    }

    char response[sizeof(bool) + PATH_MAX];
    if (send(sock, path, strlen(path) + 1, 0) == -1) {
        printf("[PRISM SANDBOX WORKAROUND] Failed to send library path, attempting to load library anyway\n");
        return dlopen(path, mode);
    }
    if (recv(sock, response, sizeof(response), 0) == -1) {
        printf("[PRISM SANDBOX WORKAROUND] Failed to receive response from launcher, attempting to load library anyway\n");
        return dlopen(path, mode);
    }

    return dlopen(path, mode);
}

DYLD_INTERPOSE(dlopen_new, dlopen);