#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"
#include "terminal.h"
#include "protocol.h"

#include "global_sockets.h"
#include "server_logic.h"



// Ensure NUM_WORKERS and BUFFER_SIZE are included
#ifndef NUM_WORKERS
#define NUM_WORKERS 10
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif




#include <inttypes.h> // For PRIu64

int TEST_wait_for_udp_messages(TcsSocket udp_server, TerminalWorker workers[], size_t num_workers) {
    size_t alive_workers = num_workers; // Start with all workers alive

    while (alive_workers > 0) {
        WorkerMessage message;
        struct TcsAddress sender_address;
        size_t bytes_received;

        // Receive UDP message
        if (tcs_receive_from(udp_server, (uint8_t*)&message, sizeof(message), TCS_NO_FLAGS, &sender_address, &bytes_received) != TCS_SUCCESS || bytes_received != sizeof(message)) {
            fprintf(stderr, "Failed to receive UDP message.\n");
            continue; // Retry on failure
        }

        // Validate the magic number
        if (message.magic != WORKER_MESSAGE_MAGIC) {
            fprintf(stderr, "Received message with invalid magic number.\n");
            continue; // Ignore invalid messages
        }

        // Validate worker ID
        if (message.worker_id < 0 || (size_t)message.worker_id >= num_workers) {
            fprintf(stderr, "Invalid worker ID received in UDP message: %d.\n", message.worker_id);
            continue;
        }

        TerminalWorker* worker = &workers[message.worker_id];

        // Handle the message based on its type
        switch (message.type) {
            case WORKER_MSG_UPDATE: {
                size_t bytes_to_read = message.payload.update.bytes_written;
                char buffer[BUFFER_SIZE];

                while (bytes_to_read > 0) {
                    size_t chunk_size = bytes_to_read < BUFFER_SIZE ? bytes_to_read : BUFFER_SIZE;
                    size_t bytes_read = fread(buffer, 1, chunk_size, worker->process);

                    if (bytes_read == 0) {
                        if (feof(worker->process)) {
                            fprintf(stdout, "Worker %d: Reached EOF while reading from process.\n", message.worker_id);
                            break;
                        }
                        if (ferror(worker->process)) {
                            fprintf(stderr, "Worker %d: Error while reading from process.\n", message.worker_id);
                            clearerr(worker->process); // Clear error and continue
                            break;
                        }
                    }

                    fwrite(buffer, 1, bytes_read, stdout); // Print to stdout
                    bytes_to_read -= bytes_read;
                }

                fprintf(stdout, "Worker %d: Processed %" PRIu64 " bytes from process.\n",
                        message.worker_id, message.payload.update.bytes_written);
                break;
            }
            case WORKER_MSG_TASK_DONE:
                fprintf(stdout, "Worker %d: Task completed with status code %d.\n",
                        message.worker_id, message.payload.task_done.status_code);
                break;

            case WORKER_MSG_TASK_INIT:
                fprintf(stdout, "Worker %d: Task initialized with ID %d.\n",
                        message.worker_id, message.payload.task_init.task_id);
                break;

            case WORKER_MSG_SHUTDOWN:
                fprintf(stdout, "Worker %d: Shutdown message received. Marking as terminated.\n", message.worker_id);

                // Mark worker as shutdown and decrement alive workers count
                if (worker->tcp_socket != TCS_NULLSOCKET) {
                    tcs_destroy(&worker->tcp_socket);
                    worker->tcp_socket = TCS_NULLSOCKET;
                }
                if (worker->process) {
                    PCLOSE(worker->process);
                    worker->process = NULL;
                }
                alive_workers--;
                break;

            default:
                fprintf(stderr, "Worker %d: Received unknown message type %d.\n",
                        message.worker_id, message.type);
        }
    }

    fprintf(stdout, "All workers have shut down. Exiting.\n");
    return 0; // Success
}




// int main(int argc, char* argv[]) {
int main() {
    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s <tcp_port> <udp_port>\n", argv[0]);
    //     return EXIT_FAILURE;
    // }
    //
    // uint16_t tcp_port = (uint16_t)atoi(argv[1]);
    // uint16_t udp_port = (uint16_t)atoi(argv[2]);

    if (tcs_lib_init() != TCS_SUCCESS) {
        fprintf(stderr, "Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }

    ServerState state;
    // if (initialize_server_state(&state, tcp_port, udp_port) != 0) {
    if (initialize_server_state(&state) != 0) {
        fprintf(stderr, "Failed to initialize server state.\n");
        if (tcs_lib_free() != TCS_SUCCESS) {
            fprintf(stderr, "Could not free tinycsocket.\n");
        }
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Server running. TCP: %u, UDP: %u\n", state.tcp_port, state.udp_port);

    TerminalWorker workers[NUM_WORKERS] = {0};
    if(inlize_workers(workers,NUM_WORKERS,&state)) 
        goto exit_fail;

    if(TEST_wait_for_udp_messages(global_udp_socket,workers,NUM_WORKERS))
        goto exit_fail;

    // Cleanup and close resources
    cleanup_workers(workers,NUM_WORKERS);
    cleanup_server_state(&state);

    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Could not free tinycsocket.\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "test passed.\n");
    return EXIT_SUCCESS;

    //FAIL MODE:
exit_fail:

    // Cleanup and close resources
    cleanup_workers(workers,NUM_WORKERS);
    cleanup_server_state(&state);

    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Could not free tinycsocket.\n");
    }
    return EXIT_FAILURE;

    
}
