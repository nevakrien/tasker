#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Structure for UDP message
typedef struct {
    uint32_t worker_id;       // Unique ID for the worker
    uint64_t bytes_written;   // Total bytes written to stdout
} WorkerUdpMessage;

#endif // PROTOCOL_H
