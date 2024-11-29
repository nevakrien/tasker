#ifndef GLOBAL_SOCKET_H
#define GLOBAL_SOCKET_H

#include "tinycsocket.h"

//we need this crap so that there is no possible way to drop a socket connection on open

// Global variables for registered sockets
extern TcsSocket global_tcp_socket;
extern TcsSocket global_udp_socket;


// Function to set up the interrupt handler
void setup_interrupt_handler(/*sockets*/void);

#endif // GLOBAL_SOCKET_H
