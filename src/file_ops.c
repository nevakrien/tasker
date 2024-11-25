#include "file_ops.h"
#include <threads.h>
#include <stdlib.h>

int copy_file(CopyTask* task) {
    char buffer[BUFSIZ]; // Temporary buffer for copying
    size_t bytes_read, bytes_written;

    FILE* in_file = fopen(task->in_path, "rb");
    if (in_file == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE* out_file = fopen(task->out_path, "wb");
    if (out_file == NULL) {
        perror("Error opening output file");
        fclose(in_file);
        return 1;
    }

    for (;!*task->global_err;) {
        // Read from the input file
        bytes_read = fread(buffer, 1, BUFSIZ, in_file);
        if (bytes_read <= 0) {
            // Check for EOF or an error
            if (feof(in_file)) {
            	fclose(in_file);
    			fclose(out_file);
                return 0; // Successfully copied all data
            }
            if (ferror(in_file)) {
                perror("Error reading from input file");
                goto exit_err;
            }
        }

        // Write to the output file
        bytes_written = fwrite(buffer, 1, bytes_read, out_file);
        if (bytes_written < bytes_read) {
            perror("Error writing to output file");
            goto exit_err;
        }
    }

    fclose(in_file);
    fclose(out_file);
    return 2; // Stopped due to global error

exit_err:
    fclose(in_file);
    fclose(out_file);
    return 1; // Error
}

static int copy_file_worker(void* data){
	CopyTask* task = (CopyTask*)data;
	int ans = copy_file(task);
	if(ans == 1) {
		*task->global_err = true;
	}
	return ans;
}

// Parallel copy function
int parallel_copy_file(const char **in, const char **out, size_t num_files) {
    CopyTask *tasks = malloc(sizeof(CopyTask) * num_files);
    thrd_t *threads = malloc(sizeof(thrd_t) * num_files);
    volatile bool global_err = false;

    // Initialize tasks
    for (size_t i = 0; i < num_files; i++) {
        tasks[i] = (CopyTask) {
        	in[i],
        	out[i],
        	&global_err
        };
    }

    // Start threads
    size_t num_threads = 0;
    for (size_t i = 0; i < num_files; i++,num_threads++) {
        if (thrd_create(&threads[i], copy_file_worker, &tasks[i]) != thrd_success) {
            perror("Error creating thread");
            global_err = true; // Signal global error
            goto exit_err;
        }
    }

    // Join threads
    for (size_t i = 0; i < num_threads; i++) {
        thrd_join(threads[i], NULL);
    }

    // Check for errors
    if (global_err) {
        free(tasks);
        free(threads);
        return 1;
    }

    free(tasks);
    free(threads);
    return 0;

exit_err:
    free(tasks);
    free(threads);
    return 1;
}
