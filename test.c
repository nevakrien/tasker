#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "task.h"

#define NUM_TASKS 3

int main() {
    // Task names and commands
    const char *commands[] = {
        "echo Task1 && sleep 1",
        "echo Task2 && sleep 2 && echo Done",
        "echo Task3 && sleep 1 && ls"
    };

    // Array of TaskHandles
    TaskHandle tasks[NUM_TASKS];

    // Initialize tasks with only task names set
    for (int i = 0; i < NUM_TASKS; i++) {
        snprintf(tasks[i].task_name, sizeof(tasks[i].task_name), "task%d", i + 1);
        tasks[i].process = NULL;
        tasks[i].error_file = NULL;
        tasks[i].updates_file = NULL;
    }

    // Start all tasks
    for (int i = 0; i < NUM_TASKS; i++) {
        if (init_start_task(&tasks[i], commands[i]) == 0) {
            printf("Started task: %s\n", tasks[i].task_name);
        } else {
            fprintf(stderr, "Failed to start task: %s\n", tasks[i].task_name);
        }
    }

    // Monitor tasks
    char buffer[FILENAME_MAX];
    int tasks_remaining = NUM_TASKS;
    while (tasks_remaining > 0) {
        for (int i = 0; i < NUM_TASKS; i++) {
            int result = retrieve_task_data(&tasks[i], buffer, sizeof(buffer));
            if (result > 0) {
                printf("[%s Update]: %s", tasks[i].task_name, buffer);
            } else if (result == 0) {
                printf("[%s]: Still running...\n", tasks[i].task_name);
            } else if (result == EOF) {
                printf("[%s]: Task completed.\n", tasks[i].task_name);
                close_task(&tasks[i]);
                tasks[i].process = NULL; // Mark task as closed
                tasks_remaining--;
            } else {
                fprintf(stderr, "[%s]: Error retrieving task data: %s\n", tasks[i].task_name, strerror(errno));
                close_task(&tasks[i]);
                tasks[i].process = NULL; // Mark task as closed
                tasks_remaining--;
            }
        }
        sleep(1); // Poll periodically
    }

    printf("All tasks completed.\n");
    return 0;
}
