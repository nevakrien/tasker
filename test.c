#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"
#include "terminal.h"

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


// Function to clean up the server state
void cleanup_server_state(ServerState* state) {
    tcs_destroy(&state->tcp_server);
    tcs_destroy(&state->udp_server);
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

    // Start all workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers[i].process = start_worker(i, tcp_port, udp_port);
        if (!workers[i].process) {
            fprintf(stderr, "Failed to start worker %d.\n", i);
            for (int j = 0; j < i; j++) {
                pclose(workers[j].process);
            }
            cleanup_server_state(&state);
            if (tcs_lib_free() != TCS_SUCCESS) {
                fprintf(stderr, "Could not free tinycsocket.\n");
            }
            return EXIT_FAILURE;
        }
    }

    // Map workers to TCP connections
    for (int i = 0; i < NUM_WORKERS; i++) {
        TcsSocket worker_socket = TCS_NULLSOCKET;
        if (tcs_accept(state.tcp_server, &worker_socket, NULL) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to accept connection from worker.\n");
            return EXIT_FAILURE;
        }

        // Receive worker ID from the connection
        uint32_t worker_id;
        size_t received;
        if (tcs_receive(worker_socket, (uint8_t*)&worker_id, sizeof(worker_id), TCS_NO_FLAGS, &received) != TCS_SUCCESS || received != sizeof(worker_id)) {
            fprintf(stderr, "Failed to receive worker ID.\n");
            tcs_destroy(&worker_socket);
            return EXIT_FAILURE;
        }

        if (worker_id >= NUM_WORKERS) {
            fprintf(stderr, "Invalid worker ID received: %u.\n", worker_id);
            tcs_destroy(&worker_socket);
            return EXIT_FAILURE;
        }

        // Match socket to the process
        workers[worker_id].tcp_socket = worker_socket;
        fprintf(stdout, "Worker %u connected successfully.\n", worker_id);
    }

    // Cleanup and close resources
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (workers[i].tcp_socket != TCS_NULLSOCKET) {
            tcs_destroy(&workers[i].tcp_socket);
        }
        if (workers[i].process) {
            pclose(workers[i].process);
        }
    }

    cleanup_server_state(&state);

    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Could not free tinycsocket.\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "test passed.\n");

    return EXIT_SUCCESS;
}
