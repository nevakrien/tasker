#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "shared_file.h"
#include "task.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <task_name> <command>\n", argv[0]);
        return 1;
    }

    const char *task_name = argv[1];
    const char *command = argv[2];


    // Open updates and error files using shared file access
    FILE *updates_file = open_updates_file(task_name, "w");
    if (!updates_file) {
        perror("Failed to open updates file");
        return 1;
    }

    FILE *error_file = open_error_file(task_name, "w");
    if (!error_file) {
        perror("Failed to open error code file");
        fclose(updates_file);
        return 1;
    }

    fprintf(updates_file, "Task started: %s\n", command);
    fflush(updates_file);

    // Run the command using popen to capture its output
    FILE *cmd_output = POPEN(command, "r");
    if (!cmd_output) {
        perror("Failed to execute command");
        fprintf(error_file, "1\n"); // Write error code
        fclose(updates_file);
        fclose(error_file);
        return 1;
    }

    int ch;
    while ((ch = fgetc(cmd_output)) != EOF) {
        fputc(ch, updates_file); // Write character to the updates file
    }
    fflush(updates_file); // Ensure updates are written immediately


    int exit_code = POCLOSE(cmd_output);
    fprintf(error_file, "%d\n", exit_code); // Write final status code

    fclose(updates_file);
    fclose(error_file);

    return exit_code;
}
