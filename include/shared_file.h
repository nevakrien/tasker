#ifndef SHARED_FILE_H
#define SHARED_FILE_H

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>


static inline FILE* open_shared_file(const char *filename, const char *mode) {
    return fopen(filename, mode); //windows has exclusivity as deafualt (which is anoying and we may want to go around)
}

#else
#include <stdio.h>

static inline FILE* open_shared_file(const char *filename, const char *mode) {
    return fopen(filename, mode); 
}

/*

we will probably want this for some atomic file access in the future.
assuming exclusivty and making linux comply is probably smarter
so instead of this async style of coding
we can go ahead and do something that has a work queue 
and we just keep track of what tasks are possible to do in the file system

this allows us to play the blocking game on tasks which is probably the way to go.

*/

// #include <stdio.h>
// #include <stdlib.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <errno.h>

// static inline FILE* open_shared_file(const char *filename, const char *mode) {
//     FILE *file = fopen(filename, mode);
//     if (!file) {
//         perror("Failed to open file");
//         return NULL;
//     }

//     int fd = fileno(file);
//     if (fd == -1) {
//         perror("Failed to get file descriptor");
//         fclose(file);
//         return NULL;
//     }

//     struct flock lock;
//     if (*mode == 'w') {
//         lock.l_type = F_WRLCK; // Exclusive lock for writing
//     } else if (*mode == 'r') {
//         if (*(mode + 1) == '+') {
//             lock.l_type = F_WRLCK; // Exclusive lock for read-write (r+)
//         } else {
//             lock.l_type = F_RDLCK; // Shared lock for read-only
//         }
//     } else {
//         fprintf(stderr, "Invalid mode: %s\n", mode);
//         fclose(file);
//         return NULL;
//     }

//     lock.l_whence = SEEK_SET; // Lock from the start of the file
//     lock.l_start = 0;         // Start of the file
//     lock.l_len = 0;           // Lock the whole file (0 means until EOF)

//     if (fcntl(fd, F_SETLK, &lock) == -1) {
//         if (errno == EACCES || errno == EAGAIN) {
//             fprintf(stderr, "File is already locked by another process\n");
//         } else {
//             perror("Failed to set lock");
//         }
//         fclose(file);
//         return NULL;
//     }

//     return file;
// }


#endif



#endif // SHARED_FILE_H
