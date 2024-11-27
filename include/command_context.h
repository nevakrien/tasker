#ifndef COMMAND_CONTEXT_H
#define COMMAND_CONTEXT_H
#include "cross_pipe.h"
#include "utils.h"
#include <stdbool.h>

#define TASK_CHUNK_SIZE (2*BUFSIZ)

#define ECHO_COMMAND(x) ("echo "x" && "x)
#define ECHO_COMMAND_DONE(x) ("echo "x" && "x" && echo done")

typedef struct CMDOutPut CMDOutPut;
struct CMDOutPut{
	char data[TASK_CHUNK_SIZE];
	size_t pos;
	CMDOutPut* next;
};

typedef struct {
	CPipe pipe;
	CMDOutPut base;
	CMDOutPut* tail;
	int statuscode;
}RuningTask;

static inline void init_cmd_output(CMDOutPut* cmd){
	cmd->pos = 0;
	cmd->next =NULL;
}

static inline void print_cmd_output(CMDOutPut* cmd){
	for(;cmd!=NULL;cmd=cmd->next){
		for(size_t i=0;i<cmd->pos;i++){
			putchar(cmd->data[i]);
		}
	}
}

static inline bool task_init(RuningTask* task,const char* command) {
	task->tail = &task->base;
	init_cmd_output(task->tail);

	task->pipe = cpipe_open(command, "r");
	if (task->pipe.stream == NULL){
		return false;
	}
	return true;
}

static inline bool check_on_task(RuningTask* task){
	if(task->pipe.stream==NULL){
		return true;
	}

	int available = cpipe_available_bytes(&task->pipe);
	if(available==EOF){
		return true;
	}

	if(available<0){
		task->statuscode = cpipe_close(&task->pipe);
		return true;
	}

	if(available==0){
		if(cpipe_check(&task->pipe))
			return true;
		return false;
	}

	//make sure we have room
	if(task->tail->pos==TASK_CHUNK_SIZE){
		task->tail->next = null_check(malloc(sizeof(CMDOutPut)));
		task->tail = task->tail->next;
		init_cmd_output(task->tail); 
	}

	size_t to_read = TASK_CHUNK_SIZE-task->tail->pos;
	to_read  = min_int_size_t(to_read,available);
	ASSERT(to_read>0);

	task->tail->pos+=cpipe_read(
		&task->pipe,
		task->tail->data,
		to_read
	);

	if(cpipe_error(&task->pipe)){
		return true;
	}

	return check_on_task(task);

}  

#endif // COMMAND_CONTEXT_H
