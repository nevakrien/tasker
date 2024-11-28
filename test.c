#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "task.h"


int main() {
    // Task names and commands
    const char *commands[] = {
        "echo Task1 && sleep 1",
        "echo Task2 && sleep 0.1 && echo Done",
        // "ls",
        "zig cc -O2 src/hello_world.c -ohello_world"
    };

    // Array of TaskHandles
    TaskHandle tasks[3];

    // Initialize tasks with only task names set
    for (size_t i = 0; i < 3; i++) {
        // memset(&tasks[i], 0, sizeof(TaskHandle)); // Zero all fields
        tasks[i]=(TaskHandle){0};
        snprintf(tasks[i].task_name, sizeof(tasks[i].task_name), "task%ld", i + 1);
    }

    // Start all tasks
    for (size_t i = 0; i < 3; i++) {
        if (init_start_task(&tasks[i], commands[i]) == 0) {
            printf("Started task: %s\n", tasks[i].task_name);
        } else {
            fprintf(stderr, "Failed to start task: %s\n", tasks[i].task_name);
        }
    }

    // Monitor tasks
    char buffer[FILENAME_MAX];
    int tasks_remaining = 3;
    while (tasks_remaining > 0) {
        for (size_t i = 0; i < 3; i++) {
            // if(tasks[i].process==NULL){
            //     continue;
            // }
            int result = retrieve_task_data(&tasks[i], buffer, sizeof(buffer));
            if (result > 0) {
                printf("[%s Update]: %s", tasks[i].task_name, buffer);
            } else if (result == 0) {
                
            } else if (result == EOF) {
                printf("[%s]: Task completed.\n", tasks[i].task_name);
                // close_task(&tasks[i]);
                tasks_remaining--;
            } else {
                fprintf(stderr, "[%s]: Error retrieving task data: %s\n", tasks[i].task_name, strerror(errno));
                // close_task(&tasks[i]);
                tasks_remaining--;
            }

            // // Check task status
            // int status = check_task_status(&tasks[i]);
            // if (status > 0) {
            //     printf("[%s]: Task completed with status %d.\n", tasks[i].task_name, status);
            //     close_task(&tasks[i]);
            //     tasks[i].process = NULL; // Mark task as closed
            //     tasks_remaining--;
            // } else if (status == 0) {
            //     printf("[%s]: Still running...\n", tasks[i].task_name);
            // } else {
            //     fprintf(stderr, "[%s]: Error checking status: %s\n", tasks[i].task_name, strerror(errno));
            //     close_task(&tasks[i]);
            //     tasks[i].process = NULL; // Mark task as closed
            //     tasks_remaining--;
            // }
        
        }
        sleep(1); // Poll periodically
    }

    printf("All tasks completed.\n");
    return 0;
}
