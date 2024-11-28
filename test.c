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

int initialize_server_state(ServerState* state, uint16_t tcp_port, uint16_t udp_port) {
    state->tcp_port = tcp_port;
    state->udp_port = udp_port;

    // Initialize sockets to TCS_NULLSOCKET
    state->tcp_server = TCS_NULLSOCKET;
    state->udp_server = TCS_NULLSOCKET;

    fprintf(stderr, "Initializing TCP server...\n");

    // Create and bind the TCP server
    if (tcs_create(&state->tcp_server, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create TCP server socket. Check system resources and port conflicts.\n");
        return -1;
    }

    fprintf(stderr, "TCP server socket created successfully.\n");

    if (tcs_bind(state->tcp_server, tcp_port) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to bind TCP server to port %u. Port might already be in use.\n", tcp_port);
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    fprintf(stderr, "TCP server bound to port %u successfully.\n", tcp_port);

    if (tcs_listen(state->tcp_server, NUM_WORKERS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to listen on TCP server.\n");
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    fprintf(stderr, "TCP server is now listening.\n");

    fprintf(stderr, "Initializing UDP server...\n");

    // Create and bind the UDP server
    if (tcs_create(&state->udp_server, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create UDP server socket. Check system resources.\n");
        tcs_destroy(&state->tcp_server);
        return -1;
    }

    fprintf(stderr, "UDP server socket created successfully.\n");

    if (tcs_bind(state->udp_server, udp_port) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to bind UDP server to port %u. Port might already be in use.\n", udp_port);
        tcs_destroy(&state->tcp_server);
        tcs_destroy(&state->udp_server);
        return -1;
    }

    fprintf(stderr, "UDP server bound to port %u successfully.\n", udp_port);
    return 0; // Success
}

int inlize_workers(TerminalWorker workers[],size_t num_workers,ServerState* state){
    // Start all workers
    for (size_t i = 0; i < num_workers; i++) {
        workers[i].tcp_socket = TCS_NULLSOCKET;
        workers[i].process = NULL;
    }

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
        if (tcs_accept(state->tcp_server, &worker_socket, NULL) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to accept connection from worker.\n");
            return -3;
        }

        // Receive worker ID from the connection
        uint32_t worker_id;
        size_t received;
        if (tcs_receive(worker_socket, (uint8_t*)&worker_id, sizeof(worker_id), TCS_NO_FLAGS, &received) != TCS_SUCCESS || received != sizeof(worker_id)) {
            fprintf(stderr, "Failed to receive worker ID.\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        if (worker_id >= num_workers) {
            fprintf(stderr, "Invalid worker ID received: %u.\n", worker_id);
            tcs_destroy(&worker_socket);
            return -1;
        }

        // Match socket to the process
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
            pclose(workers[i].process);
        }
    }

}


int TEST_wait_for_udp_messages(TcsSocket udp_server, size_t num_workers) {
    const size_t total_messages = num_workers * 3; // 3 messages per worker
    size_t messages_received = 0;

    while (messages_received < total_messages) {
        WorkerUdpMessage message;
        struct TcsAddress sender_address;
        size_t bytes_received;

        // Receive UDP message
        if (tcs_receive_from(udp_server, (uint8_t*)&message, sizeof(message), TCS_NO_FLAGS, &sender_address, &bytes_received) != TCS_SUCCESS || bytes_received != sizeof(message)) {
            fprintf(stderr, "Failed to receive UDP message.\n");
            continue; // Retry on failure
        }

        // Validate worker ID
        if (message.worker_id >= num_workers) {
            fprintf(stderr, "Invalid worker ID received in UDP message: %u.\n", message.worker_id);
            continue;
        }

        // Log the message
        fprintf(stdout, "Received UDP message from Worker %u: bytes_written=%" PRIu64 "\n",
                message.worker_id, message.bytes_written);

        messages_received++;
    }

    fprintf(stdout, "All %zu UDP messages received.\n", total_messages);
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

    fprintf(stdout, "Server running. TCP: %u, UDP: %u\n", tcp_port, udp_port);

    TerminalWorker workers[NUM_WORKERS] = {0};
    if(inlize_workers(workers,NUM_WORKERS,&state)) 
        goto exit_fail;

    if(TEST_wait_for_udp_messages(state.udp_server,NUM_WORKERS))
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
