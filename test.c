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

#include "intrupt_cleanup.h"


// Ensure NUM_WORKERS and BUFFER_SIZE are included
#ifndef NUM_WORKERS
#define NUM_WORKERS 10
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

// Global state struct
typedef struct {
    TcsSocket tcp_server;
    TcsSocket udp_server;
    uint16_t tcp_port;
    uint16_t udp_port;
} ServerState;

#define TCP_SERVER_TIMEOUT_MS 10000   // Timeout for TCP server receive in milliseconds
#define UDP_SERVER_TIMEOUT_MS 5000    // Timeout for UDP server receive in milliseconds
#define WORKER_CONNECTION_TIMEOUT_MS 10000 // Timeout for worker connection in milliseconds


int initialize_server_state(ServerState* state, uint16_t tcp_port, uint16_t udp_port) {
    state->tcp_port = tcp_port;
    state->udp_port = udp_port;

    // Initialize sockets to TCS_NULLSOCKET
    state->tcp_server = TCS_NULLSOCKET;
    state->udp_server = TCS_NULLSOCKET;

    fprintf(stderr, "Initializing TCP server...\n");

    // Create the TCP server socket
    if (tcs_create(&state->tcp_server, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create TCP server socket. Check system resources and port conflicts.\n");
        return -1;
    }

    // Set timeout for TCP server receive
    if (tcs_set_receive_timeout(state->tcp_server, TCP_SERVER_TIMEOUT_MS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to set receive timeout for TCP server.\n");
        tcs_destroy(&state->tcp_server);
        return -1;
    }
    fprintf(stderr, "Timeout for TCP server set to %d ms.\n", TCP_SERVER_TIMEOUT_MS);

    // Bind the TCP server socket
    if (tcs_bind(state->tcp_server, tcp_port) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to bind TCP server to port %u. Port might already be in use.\n", tcp_port);
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    fprintf(stderr, "TCP server bound to port %u successfully.\n", tcp_port);

    // Start listening on the TCP server
    if (tcs_listen(state->tcp_server, NUM_WORKERS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to listen on TCP server.\n");
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    fprintf(stderr, "TCP server is now listening.\n");

    fprintf(stderr, "Initializing UDP server...\n");

    // Create the UDP server socket
    if (tcs_create(&state->udp_server, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create UDP server socket. Check system resources.\n");
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    // Set timeout for UDP server receive
    if (tcs_set_receive_timeout(state->udp_server, UDP_SERVER_TIMEOUT_MS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to set receive timeout for UDP server.\n");
        tcs_destroy(&state->tcp_server);
        tcs_destroy(&state->udp_server);
        return -1;
    }
    fprintf(stderr, "Timeout for UDP server set to %d ms.\n", UDP_SERVER_TIMEOUT_MS);

    // Bind the UDP server socket
    if (tcs_bind(state->udp_server, udp_port) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to bind UDP server to port %u. Port might already be in use.\n", udp_port);
        tcs_destroy(&state->tcp_server);
        tcs_destroy(&state->udp_server);
        return -1;
    }

    fprintf(stderr, "UDP server bound to port %u successfully.\n", udp_port);
    return 0; // Success
}


int inlize_workers(TerminalWorker workers[], size_t num_workers, ServerState* state) {
    // Initialize all worker entries
    for (size_t i = 0; i < num_workers; i++) {
        workers[i].tcp_socket = TCS_NULLSOCKET;
        workers[i].process = NULL;
    }

    // Start worker processes
    for (size_t i = 0; i < num_workers; i++) {
        workers[i].process = start_worker(i, state->tcp_port, state->udp_port);
        if (!workers[i].process) {
            fprintf(stderr, "Failed to start worker %lu.\n", i);
            return -1;
        }
    }

    // Map workers to TCP connections
    for (size_t i = 0; i < num_workers; i++) {
        TcsSocket worker_socket = TCS_NULLSOCKET;

        // Accept a connection from a worker
        if (tcs_accept(state->tcp_server, &worker_socket, NULL) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to accept connection from worker.\n");
            return -3;
        }

        // Set timeout for the worker socket
        if (tcs_set_receive_timeout(worker_socket, WORKER_CONNECTION_TIMEOUT_MS) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to set receive timeout for worker socket.\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        // Receive worker ID from the connection
        worker_id_t worker_id;
        size_t received;
        if (tcs_receive(worker_socket, (uint8_t*)&worker_id, sizeof(worker_id), TCS_NO_FLAGS, &received) != TCS_SUCCESS || received != sizeof(worker_id)) {
            fprintf(stderr, "Failed to receive worker ID.\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        // Validate worker ID
        if (worker_id >= (worker_id_t)num_workers) {
            fprintf(stderr, "Invalid worker ID received: %u.\n", worker_id);
            tcs_destroy(&worker_socket);
            return -1;
        }

        // Match the socket to the worker process
        workers[worker_id].tcp_socket = worker_socket;
        fprintf(stdout, "Worker %u connected successfully.\n", worker_id);
    }

    return 0;
}


// Function to clean up the server state
void cleanup_server_state(ServerState* state) {
    tcs_destroy(&state->tcp_server);
    tcs_destroy(&state->udp_server);
}

void cleanup_workers(TerminalWorker workers[],size_t num_workers){
    for (size_t i = 0; i < num_workers; i++) {
        if (workers[i].tcp_socket != TCS_NULLSOCKET) {
            tcs_destroy(&workers[i].tcp_socket);
        }
        if (workers[i].process) {
            PCLOSE(workers[i].process);
        }
    }

}


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




int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <tcp_port> <udp_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t tcp_port = (uint16_t)atoi(argv[1]);
    uint16_t udp_port = (uint16_t)atoi(argv[2]);

    if (tcs_lib_init() != TCS_SUCCESS) {
        fprintf(stderr, "Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }

    ServerState state;
    if (initialize_server_state(&state, tcp_port, udp_port) != 0) {
        fprintf(stderr, "Failed to initialize server state.\n");
        if (tcs_lib_free() != TCS_SUCCESS) {
            fprintf(stderr, "Could not free tinycsocket.\n");
        }
        return EXIT_FAILURE;
    }

    setup_interrupt_handler();
    tcp_socket_to_cleanup = state.tcp_server;
    udp_socket_to_cleanup = state.udp_server;

    fprintf(stdout, "Server running. TCP: %u, UDP: %u\n", tcp_port, udp_port);

    TerminalWorker workers[NUM_WORKERS] = {0};
    if(inlize_workers(workers,NUM_WORKERS,&state)) 
        goto exit_fail;

    if(TEST_wait_for_udp_messages(state.udp_server,workers,NUM_WORKERS))
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
