#ifndef TASKS_H
#define TASKS_H

typedef enum {
	ReadFile,
	WriteFile,
	MakeDest,
	ReadSource,

} TaskType;

typedef struct {
	TaskType type;
	int fd;
} Task;

#endif // TASKS_H
