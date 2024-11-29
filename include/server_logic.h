#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "tinycsocket.h"
#include "terminal.h"
#include "protocol.h"
#include "global_sockets.h"

// Global state struct
typedef struct {
    // TcsSocket tcp_server;
    // TcsSocket udp_server;
    uint16_t tcp_port;
    uint16_t udp_port;
} ServerState;

#define BACKLOG 100
#define TCP_SERVER_TIMEOUT_MS 10000   // Timeout for TCP server receive in milliseconds
#define UDP_SERVER_TIMEOUT_MS 5000    // Timeout for UDP server receive in milliseconds
#define WORKER_CONNECTION_TIMEOUT_MS 10000 // Timeout for worker connection in milliseconds

// #define SAFE_PORT_RANGE_START 30000
// #define SAFE_PORT_RANGE_END 39999

//use just 100 ports should be fine
#define SAFE_PORT_RANGE_START 4242
#define SAFE_PORT_RANGE_END 6969	

// #define DEBUG_LOG(x, ...) fprintf(stderr,x,##__VA_ARGS__)
#define DEBUG_LOG(x, ...) 

static int32_t open_find_port(TcsSocket* socket, uint16_t start_port, uint16_t end_port) {
    if (!socket) {
        fprintf(stderr, "Invalid socket argument to open_find_port.\n");
        return TCS_ERROR_INVALID_ARGUMENT;
    }

    uint16_t port = start_port;
    while (port <= end_port) {
        DEBUG_LOG( "Attempting to bind to port %u...\n", port);
        
        int code;
        if ((code=tcs_bind(*socket, port)) == TCS_SUCCESS) {
            DEBUG_LOG( "Successfully bound to port %u.\n", port);
            return (int32_t)port; // Return the successfully bound port
        }
        DEBUG_LOG( "got return code %d\n", code);
        port++;
    }

    DEBUG_LOG( "Failed to find an available port in range %u-%u.\n", 
            start_port, end_port);
    return TCS_ERROR_KERNEL; // Indicate failure to bind
}

static int initialize_server_state(/*sockets*/ ServerState* state) {
    setup_interrupt_handler();
    // state->tcp_port = tcp_port;
    // state->udp_port = udp_port;

    // Initialize sockets to TCS_NULLSOCKET
    global_tcp_socket = TCS_NULLSOCKET;
    global_udp_socket = TCS_NULLSOCKET;

    DEBUG_LOG( "Initializing TCP server...\n");
    if (tcs_lib_init() != TCS_SUCCESS) {
        fprintf(stderr,"Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }
    
    // Create the TCP server socket
    if (tcs_create(&global_tcp_socket, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create TCP server socket. Check system resources and port conflicts.\n");
        return -1;
    }


    // Set timeout for TCP server receive
    if (tcs_set_receive_timeout(global_tcp_socket, TCP_SERVER_TIMEOUT_MS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to set receive timeout for TCP server.\n");
        tcs_destroy(&global_tcp_socket);
        return -1;
    }
    DEBUG_LOG( "Timeout for TCP server set to %d ms.\n", TCP_SERVER_TIMEOUT_MS);

    int32_t tcp_port = open_find_port(&global_tcp_socket,SAFE_PORT_RANGE_START,SAFE_PORT_RANGE_END);
    if(tcp_port<0){
    	fprintf(stderr, "Failed to create TCP server socket. Check system resources and port conflicts.\n");
        return -1;
    } 
    state->tcp_port=tcp_port;


    DEBUG_LOG( "TCP server bound to port %u successfully.\n", state->tcp_port);

    // Start listening on the TCP server
    if (tcs_listen(global_tcp_socket, BACKLOG) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to listen on TCP server.\n");
        tcs_destroy(&global_tcp_socket);
        return -1;
    }

    DEBUG_LOG( "TCP server is now listening.\n");

    DEBUG_LOG( "Initializing UDP server...\n");

    // Create the UDP server socket
    if (tcs_create(&global_udp_socket, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create UDP server socket. Check system resources.\n");
        tcs_destroy(&global_tcp_socket);
        return -1;
    }

    // Set timeout for UDP server receive
    if (tcs_set_receive_timeout(global_udp_socket, UDP_SERVER_TIMEOUT_MS) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to set receive timeout for UDP server.\n");
        tcs_destroy(&global_tcp_socket);
        tcs_destroy(&global_udp_socket);
        return -1;
    }
    DEBUG_LOG( "Timeout for UDP server set to %d ms.\n", UDP_SERVER_TIMEOUT_MS);

    int32_t udp_port = open_find_port(&global_udp_socket,state->tcp_port+1,SAFE_PORT_RANGE_END);
    if(udp_port<0){
    	fprintf(stderr, "Failed to bind UDP server to port %u. Port might already be in use.\n", udp_port);
        tcs_destroy(&global_tcp_socket);
        tcs_destroy(&global_udp_socket);
        return -1;
    } 
    state->udp_port=udp_port;

    DEBUG_LOG( "UDP server bound to port %u successfully.\n", state->udp_port);
    return 0; // Success
}


static int inlize_workers(/*sockets*/ TerminalWorker workers[], size_t num_workers, ServerState* state) {
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

        int code;
        // Accept a connection from a worker
        if ((code=tcs_accept(global_tcp_socket, &worker_socket, NULL)) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to accept connection from worker.,%d\n",code);
            return -3;
        }

        // Set timeout for the worker socket
        if (tcs_set_receive_timeout(worker_socket, WORKER_CONNECTION_TIMEOUT_MS) != TCS_SUCCESS) {
            fprintf(stderr, "Failed to set receive timeout for worker socket.\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        // Receive worker ID from the connection
        WorkerInitTcp packet;
        size_t received;
        if (tcs_receive(worker_socket, (uint8_t*)&packet, sizeof(packet), TCS_NO_FLAGS, &received) != TCS_SUCCESS || received != sizeof(packet)) {
            fprintf(stderr, "Wrong size of TCP message.\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        if(packet.magic!=WORKER_MESSAGE_MAGIC){
        	fprintf(stderr, "Wrong magic on TCP message\n");
            tcs_destroy(&worker_socket);
            return -2;
        }

        worker_id_t worker_id = packet.worker_id;

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
static void cleanup_server_state(/*sockets*/ ServerState* state) {
    (void)state;
    tcs_destroy(&global_tcp_socket);
    tcs_destroy(&global_udp_socket);
}

static void cleanup_workers(TerminalWorker workers[],size_t num_workers){
    for (size_t i = 0; i < num_workers; i++) {
        if (workers[i].tcp_socket != TCS_NULLSOCKET) {
            tcs_destroy(&workers[i].tcp_socket);
        }
        if (workers[i].process) {
            PCLOSE(workers[i].process);
        }
    }

}

#endif // SERVER_LOGIC_H
