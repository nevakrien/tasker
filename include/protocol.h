#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_COMMAND 10*FILENAME_MAX

#define WORKER_MESSAGE_MAGIC 0xDEADBEEF

typedef enum {
    WORKER_MSG_UPDATE,       // Update with new output line info
    WORKER_MSG_TASK_DONE,    // Task completion message
    WORKER_MSG_TASK_INIT,    // Task initialization message
    WORKER_MSG_SHUTDOWN      // Worker shutdown message
} WorkerMessageType;

typedef int worker_id_t;
typedef int task_id_t;

typedef struct {
    uint32_t magic; 
    worker_id_t worker_id;
} WorkerInitTcp;

typedef union {
    struct {
        size_t bytes_written;
    } update;

    struct {
        int status_code;
    } task_done;

    struct {
        task_id_t task_id;
    } task_init;

    // No struct for WORKER_MSG_SHUTDOWN, it does not require additional data
} WorkerMessagePayload;


typedef struct {
    uint32_t magic;                // Magic number for verification
    WorkerMessageType type;        // Type of the message
    worker_id_t worker_id;            // ID of the worker sending the message
    WorkerMessagePayload payload;  // Payload specific to the message type
} WorkerMessage;

#endif // PROTOCOL_H
