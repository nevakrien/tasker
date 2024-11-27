#ifndef CROSS_PIPE_H
#define CROSS_PIPE_H

#include <stdio.h>

#define CPIPW_LOG_ERROR(format, ...) //fprintf(stderr, format "\n", ##__VA_ARGS__)

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>

    typedef struct {
        FILE *stream;       // File pointer for standard I/O
        HANDLE hFile;       // File handle
    } CPipe;
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <poll.h>

    typedef struct {
        FILE *stream;       // File pointer for standard I/O
    } CPipe;
#endif

static inline int cpipe_done(CPipe* pipe){
    return feof(pipe->stream);
}

static inline int cpipe_error(CPipe* pipe){
    return ferror(pipe->stream);
}

//not error code
static inline int cpipe_check(CPipe *pipe) {
    if (pipe->stream==NULL||ferror(pipe->stream)) {
        return -1; // Error occurred
    }

    if (feof(pipe->stream)) {
        return 1; // Pipe is done (EOF)
    }

    return 0; // Pipe is not done, no error
}

// Open a pipe to execute a command
static inline CPipe cpipe_open(const char *command, const char *mode) {
    CPipe pipe;
#ifdef _WIN32
    pipe.stream = _popen(command, mode);
    if (pipe.stream) {
        pipe.hFile = (HANDLE)_get_osfhandle(_fileno(pipe.stream));
    } else {
        pipe.hFile = INVALID_HANDLE_VALUE;
    }
#else
    pipe.stream = popen(command, mode);
    if (pipe.stream) {
        // Extract the process ID from the implementation
        // Note: This assumes popen is implemented with fork()
    }
#endif
    return pipe;
}

// Check how many bytes are available to read (non-blocking)
static inline int cpipe_available_bytes(CPipe *pipe) {
#ifdef _WIN32
    if (!pipe->stream || pipe->hFile == INVALID_HANDLE_VALUE) {
        CPIPW_LOG_ERROR("Invalid pipe handle.");
        return -2; // Error
    }

    DWORD bytes_available = 0;
    if (!PeekNamedPipe(pipe->hFile, NULL, 0, NULL, &bytes_available, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE) {
            // Pipe is closed, process likely finished
            return EOF; // Done (EOF)
        }
        CPIPW_LOG_ERROR("PeekNamedPipe failed: %lu", error);
        return -2; // Error
    }

    return (int)bytes_available; // Return number of bytes available
#else
    if (pipe->stream == NULL) {
        CPIPW_LOG_ERROR("Error: stream is NULL");
        return -2; // Handle the error appropriately
    }
    int available = 0;
    int fd = fileno(pipe->stream); // Get the file descriptor
    
    // Use poll() to check if there is data in the pipe
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int poll_result = poll(&pfd, 1, 0); // Non-blocking poll
    if (poll_result > 0) {
        if (pfd.revents & POLLIN) {
            // Data is ready in the pipe, force it into the FILE
            int c = getc(pipe->stream); 
            if (c != EOF) {
                ungetc(c, pipe->stream); 
            } else {
                if (feof(pipe->stream)) {
                    return EOF;
                }
                CPIPW_LOG_ERROR("getc failed");
                return -2; // Error
            }
        }
    } else if (poll_result < 0) {
        CPIPW_LOG_ERROR("poll failed");
        return -2; // Error
    }

    if (ioctl(fd, FIONREAD, &available) == -1) {
        if (feof(pipe->stream)) {
            return EOF; // Done (EOF)
        }
        CPIPW_LOG_ERROR("ioctl failed");
        return -2; // Error
    }

    return available; // Return number of bytes available
#endif
}



// Read data from the pipe (blocking)
static inline size_t cpipe_read(CPipe *pipe, char *buffer, size_t buffer_size) {
    return fread(buffer, 1, buffer_size, pipe->stream);
}

// Write data to the pipe (blocking)
static inline size_t cpipe_write(CPipe *pipe, char *buffer, size_t buffer_size) {
    return fwrite(buffer, 1, buffer_size, pipe->stream);
}

// Close the pipe and get the exit code
static inline int cpipe_close(CPipe *pipe) {
    int exit_code;
#ifdef _WIN32
    exit_code = _pclose(pipe->stream);
    pipe->stream = NULL;
    pipe->hFile = INVALID_HANDLE_VALUE;
#else
    exit_code = pclose(pipe->stream);
    pipe->stream = NULL;
#endif
    return exit_code;
}

#endif // CROSS_PIPE_H
