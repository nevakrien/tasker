#ifndef SHARED_FILE_H
#define SHARED_FILE_H

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

static FILE* open_shared_file(const char *filename, const char *mode) {
    DWORD access = 0;
    DWORD shareMode = FILE_SHARE_READ;
    DWORD creationDisposition = OPEN_EXISTING;
    DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

    if (strchr(mode, 'w')) {
        access = GENERIC_WRITE;
        creationDisposition = CREATE_ALWAYS;
    } else if (strchr(mode, 'r')) {
        access = GENERIC_READ;
    }

    HANDLE hFile = CreateFileA(
        filename,
        access,
        shareMode,
        NULL,
        creationDisposition,
        flagsAndAttributes,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        perror("Failed to open file with shared access");
        return NULL;
    }

    int fd = _open_osfhandle((intptr_t)hFile, strchr(mode, 'r') ? _O_RDONLY : _O_WRONLY);
    if (fd == -1) {
        CloseHandle(hFile);
        perror("Failed to convert HANDLE to file descriptor");
        return NULL;
    }

    FILE *file = _fdopen(fd, mode);
    if (!file) {
        _close(fd);
        perror("Failed to convert file descriptor to FILE*");
        return NULL;
    }

    return file;
}

#else
#include <stdio.h>

static inline FILE* open_shared_file(const char *filename, const char *mode) {
    return fopen(filename, mode); // POSIX already supports shared access
}



#endif



#endif // SHARED_FILE_H
