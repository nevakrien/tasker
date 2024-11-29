#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "protocol.h"

typedef struct {
	TcsSocket tcp_socket;
	FILE* process;
} TerminalWorker;

//needs work
static inline FILE* start_worker(int worker_num,uint16_t tcp_port,uint16_t udp_port){
    // Prepare the worker command
    char worker_command[FILENAME_MAX + BUFSIZ];
    #ifdef _WIN32
        snprintf(worker_command, sizeof(worker_command), "bin\\worker.exe %d %u %u", worker_num, tcp_port,udp_port);
    #else
        snprintf(worker_command, sizeof(worker_command), "./bin/worker %d %u %u", worker_num, tcp_port,udp_port);
    #endif

    // Start the worker process
    return POPEN(worker_command, "r");
}


#endif // TERMINAL_H
