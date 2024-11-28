#ifndef INTRUPT_CLEANUP_H
#define INTRUPT_CLEANUP_H

#include "tinycsocket.h"

// Global variables for registered sockets
extern TcsSocket tcp_socket_to_cleanup;
extern TcsSocket udp_socket_to_cleanup;


// Function to set up the interrupt handler
void setup_interrupt_handler(void);

#endif // INTRUPT_CLEANUP_H
