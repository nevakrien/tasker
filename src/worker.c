#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "shared_file.h"
#include "task.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <task_name> <command> [args...]\n", argv[0]);
        return 1;
    }

    const char *task_name = argv[1];

    // Concatenate all arguments into a single command string
    size_t command_length = 0;
    for (int i = 2; i < argc; i++) {
        command_length += strlen(argv[i]) + 1; // Include space or null terminator
    }

    char *command = malloc(command_length);
    if (!command) {
        perror("Failed to allocate memory for command string");
        return 1;
    }

    command[0] = '\0'; // Initialize the command string
    for (int i = 2; i < argc; i++) {
        strcat(command, argv[i]);
        if (i < argc - 1) {
            strcat(command, " "); // Add a space between arguments
        }
    }

    // Open updates and error files using shared file access
    FILE *updates_file = open_updates_file(task_name, "w");
    if (!updates_file) {
        perror("Failed to open updates file");
        free(command);
        return 1;
    }

    FILE *error_file = open_error_file(task_name, "w");
    if (!error_file) {
        perror("Failed to open error code file");
        fclose(updates_file);
        free(command);
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
        free(command);
        return 1;
    }

    int ch;
    while ((ch = fgetc(cmd_output)) != EOF) {
        fputc(ch, updates_file); // Write character to the updates file
    }

    int exit_code = POCLOSE(cmd_output);
    fprintf(error_file, "%d\n", exit_code); // Write final status code

    fclose(updates_file);
    fclose(error_file);
    free(command);

    return exit_code;
}
