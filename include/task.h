#ifndef TASK_H
#define TASK_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "shared_file.h"

#define MAX_COMMAND 10*FILENAME_MAX

#ifdef _WIN32
#define POPEN _popen
#define POCLOSE _pclose
#else
#define POPEN popen
#define POCLOSE pclose
#endif
// Unified structure for managing tasks
typedef struct {
    char task_name[1024];
    FILE *process;       // Handle for the running process (NULL if task is only opened)
    FILE *error_file;    // For checking task status
    FILE *updates_file;  // For reading task updates
} TaskHandle;

static inline FILE *open_updates_file(const char *task_name, const char *mode) {
    if (!task_name || !mode) {
        errno = EINVAL; // Invalid arguments
        return NULL;
    }

    char filename[FILENAME_MAX];
    snprintf(filename, sizeof(filename), ".temp_%s_updates.txt", task_name);
    return open_shared_file(filename, mode);
}

static inline FILE *open_error_file(const char *task_name, const char *mode) {
    if (!task_name || !mode) {
        errno = EINVAL; // Invalid arguments
        return NULL;
    }

    char filename[FILENAME_MAX];
    snprintf(filename, sizeof(filename), ".temp_%s_error_code.txt", task_name);
    return open_shared_file(filename, mode);
}



// Initializes a TaskHandle for monitoring an existing task
static int init_open_task(TaskHandle *task) {
    if (!task || !task->task_name) {
        errno = EINVAL; // Invalid arguments
        return -1;
    }

    // Ensure TaskHandle fields are cleared
    task->process = NULL;
    task->error_file = NULL;
    task->updates_file = NULL;

    // Open shared files in read mode
    task->error_file = open_error_file(task->task_name, "r");
    task->updates_file = open_updates_file(task->task_name, "r");

    // Check if opening files failed
    if (!task->error_file && errno != ENOENT) {
        perror("Failed to open error file");
        return -1;
    }
    if (!task->updates_file && errno != ENOENT) {
        perror("Failed to open updates file");
        return -1;
    }

    return 0; // Successfully initialized
}

// Initializes a TaskHandle for a new task and starts the worker process
static int init_start_task(TaskHandle *task, const char *command) {
    if (!task || !task->task_name || !command) {
        errno = EINVAL; // Invalid arguments
        return -1;
    }

    // Ensure TaskHandle fields are cleared
    task->process = NULL;
    task->error_file = NULL;
    task->updates_file = NULL;

    // Prepare the worker command
    char worker_command[MAX_COMMAND];
    #ifdef _WIN32
        snprintf(worker_command, sizeof(worker_command), "bin\\worker.exe %s %s", task->task_name, command);
    #else
        snprintf(worker_command, sizeof(worker_command), "./bin/worker %s %s", task->task_name, command);
    #endif


    // Start the worker process
    task->process = POPEN(worker_command, "r");
    if (!task->process) {
        perror("Failed to start worker process");
        return -1;
    }

    // files might not be ready yet we may want to sleep here
    // usleep(10000); // 10ms
    return init_open_task(task); // Initialize files
}

static int wait_for_task(TaskHandle *task) {
    if (!task) return -1;

    if (task->process) {
        POCLOSE(task->process); // Close the running process, if any
        task->process=NULL;
        return 0;
    }

    return -1;
}

// Closes a task, ensuring resources are freed properly
static int close_task(TaskHandle *task) {
    if (!task) return -1;

    if (task->process) {
        POCLOSE(task->process); // Close the running process, if any
        task->process=NULL;
    }
    if (task->error_file) {
    	fclose(task->error_file);
    	task->error_file=NULL;
    }
    if (task->updates_file) {
    	fclose(task->updates_file);
    	task->error_file=NULL;
    }

    return 0;
}

// Checks the status of the task (returns 0 if running, >0 if complete, -1 on error)
static int check_task_status(TaskHandle *task) {
    if (!task->error_file) {
        task->error_file = open_error_file(task->task_name, "r");
        if (!task->error_file) {
            if (errno == ENOENT) {
                // File doesn't exist, task is still running
                return 0;
            }
            perror("Failed to open error code file");
            return -1; // Error occurred
        }
    }

    // File is open, attempt to read the status code
    char buffer[16];
    if (fgets(buffer, sizeof(buffer), task->error_file)) {
    	return 1;
        // int status_code = atoi(buffer); // Parse the status code
        // return status_code; // Task is complete with this code
    }

    if (feof(task->error_file)) {
        // EOF means task is still running
        clearerr(task->error_file); // Reset EOF for future reads
        return 0;
    }

    // Unexpected error
    perror("Error reading from error code file");
    return -1;
}

static ssize_t retrieve_task_data(TaskHandle *task, char *buffer, size_t buffer_size) {
    if (!task || !buffer || buffer_size == 0) {
        errno = EINVAL; // Invalid arguments
        return -17; // Signal error
    }

    if (!task->updates_file) {
        task->updates_file = open_updates_file(task->task_name, "r");
        if (!task->updates_file) {
            if (errno == ENOENT) {
                // File doesn't exist yet; assume task is still running
                return 0;
            }
            perror("Failed to open updates file");
            return -23; // Signal error
        }
    }

    // Attempt to read a line from the updates file
    if (fgets(buffer, buffer_size, task->updates_file)) {
        size_t bytes_read = strlen(buffer);
        return (ssize_t)bytes_read; // Return number of bytes read
    }

    // Handle EOF: Check if the task is completed
    if (feof(task->updates_file)) {
        // Clear EOF for subsequent reads
        clearerr(task->updates_file);

        // Check task status to determine if EOF means task completion
        int status = check_task_status(task);
        if (status > 0) {
            return EOF; // Signal EOF as the task is complete
        } else if (status == 0) {
            return 0; // Task is still running, no data available
        } else {
            // Error determining task status
            errno = EIO;
            return -17;
        }
    }

    // Handle unexpected errors
    perror("Error reading updates file");
    return -17; // Signal error
}

#endif // TASK_H
