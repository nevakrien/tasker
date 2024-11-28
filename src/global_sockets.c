#include "global_sockets.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

// Initialize global variables
TcsSocket global_tcp_socket;
TcsSocket global_udp_socket;

// Interrupt handler function
void handle_interrupt(int signal) {
    fprintf(stderr, "Caught signal %d. Cleaning up resources...\n", signal);

    // Close TCP socket if valid
    if (global_tcp_socket != TCS_NULLSOCKET) {
        tcs_destroy(&global_tcp_socket);
        global_tcp_socket = TCS_NULLSOCKET;
        fprintf(stderr, "TCP socket closed.\n");
    }

    // Close UDP socket if valid
    if (global_udp_socket != TCS_NULLSOCKET) {
        tcs_destroy(&global_udp_socket);
        global_udp_socket = TCS_NULLSOCKET;
        fprintf(stderr, "UDP socket closed.\n");
    }

    // Exit gracefully
    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Failed to free tinycsocket resources.\n");
    }

    exit(EXIT_SUCCESS);
}


// Function to set up the interrupt handler
void setup_interrupt_handler(void) {
     global_tcp_socket= TCS_NULLSOCKET;
     global_udp_socket= TCS_NULLSOCKET;

    signal(SIGINT, handle_interrupt);  // Ctrl+C
    signal(SIGTERM, handle_interrupt); // Termination request
#ifdef SIGQUIT
    signal(SIGQUIT, handle_interrupt); // Quit request
#endif
#ifdef SIGHUP
    signal(SIGHUP, handle_interrupt);  // Hangup signal
#endif

    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Could not free tinycsocket.\n");
    }
}
