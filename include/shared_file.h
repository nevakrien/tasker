#ifndef SHARED_FILE_H
#define SHARED_FILE_H

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>


static inline FILE* open_shared_file(const char *filename, const char *mode) {
    return fopen(filename, mode); //need to fix this to do the thing... however code works well anyway
}

#else
#include <stdio.h>

static inline FILE* open_shared_file(const char *filename, const char *mode) {
    return fopen(filename, mode); // POSIX already supports shared access
}



#endif



#endif // SHARED_FILE_H
