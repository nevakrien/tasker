#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h> // For PRIu64

#define TINYCSOCKET_IMPLEMENTATION
#include "tinycsocket.h"
#include "protocol.h"
#include "global_sockets.h"
#include "utils.h"

// #define DEBUG_LOG(x, ...) fprintf(stderr,x,##__VA_ARGS__)
#define DEBUG_LOG(x, ...) 

static size_t total_bytes_written = 0;

#define LOG_TO_STDOUT(format, ...)                                   \
    do {                                                            \
        char temp_buffer[1024];                                     \
        int bytes = snprintf(temp_buffer, sizeof(temp_buffer), format, ##__VA_ARGS__); \
        if (bytes > 0) {                                            \
            printf("%s", temp_buffer);                              \
            total_bytes_written += (uint64_t)bytes;                 \
        }                                                           \
    } while (0)

// Macro for sending WORKER_MSG_UPDATE UDP messages
#define SEND_UPDATE_MESSAGE(socket, server_addr, worker_id) \
    do {                                                                       \
        fflush(stdout);                                                        \
        WorkerMessage msg;                                                     \
        msg.magic = WORKER_MESSAGE_MAGIC;                                      \
        msg.type = WORKER_MSG_UPDATE;                                          \
        msg.worker_id = worker_id;                                             \
        msg.payload.update.bytes_written = total_bytes_written;               \
                                                                               \
        size_t udp_sent;                                                       \
        if (tcs_send_to(socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, server_addr, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) { \
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_UPDATE message.\n", worker_id); \
            total_bytes_written = 0;                                          \
        } else {                                                               \
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_UPDATE message.\n", worker_id); \
        }                                                                      \
    } while (0)




void print_usage(const char* program_name) {
    (void)program_name;
    DEBUG_LOG( "Usage: %s <worker_id> <tcp_port> <udp_port>\n", program_name);
}

//global args
worker_id_t worker_id;
struct TcsAddress server_address;
int setup(/*sockets*/ int argc,char* argv[] ){
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (tcs_lib_init() != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }

    // Parse arguments
    worker_id = atoi(argv[1]);
    uint16_t tcp_port = (uint16_t)atoi(argv[2]);
    uint16_t udp_port = (uint16_t)atoi(argv[3]);
    
    setup_interrupt_handler();

    // Initialize TCP socket
    if (tcs_create(&global_tcp_socket, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create TCP socket.\n");
        return EXIT_FAILURE;
    }

    if (tcs_connect(global_tcp_socket, "127.0.0.1", tcp_port) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to connect to server on TCP port %u.\n", tcp_port);
        tcs_destroy(&global_tcp_socket);
        return EXIT_FAILURE;
    }


    // Send worker ID over TCP
    WorkerInitTcp packet = {WORKER_MESSAGE_MAGIC,worker_id};
    if (tcs_send(global_tcp_socket, (const uint8_t*)&packet, sizeof(packet), TCS_MSG_SENDALL, NULL) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to send worker ID over TCP.\n");
        tcs_destroy(&global_tcp_socket);
        return EXIT_FAILURE;
    }

    DEBUG_LOG( "Worker %d: Sent ID to server via TCP.\n", worker_id);
    LOG_TO_STDOUT("From Worker %d: connected.\n", worker_id);

    // Initialize UDP socket
    if (tcs_create(&global_udp_socket, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create UDP socket.\n");
        tcs_destroy(&global_tcp_socket);
        return EXIT_FAILURE;
    }



    // Create server address for UDP communication
    char address_buffer[256];
    sprintf(address_buffer, "127.0.0.1:%u", udp_port);

    if (tcs_util_string_to_address(address_buffer, &server_address) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create server address for UDP communication.\n");
        tcs_destroy(&global_tcp_socket);
        tcs_destroy(&global_udp_socket);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int cleanup(/*sockets*/){
    tcs_destroy(&global_tcp_socket);
    tcs_destroy(&global_udp_socket);

    if (tcs_lib_free() != TCS_SUCCESS) {
        DEBUG_LOG( "Could not free tinycsocket.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
note we can crash on OOM. 
because on most platform malloc returns null less than half the time we go OOM.
the server should manage a crash byitself.
*/
typedef struct {
    char* memory; //cant be null
    size_t len;
    size_t capacity;
}DynamicBuffer;

static inline void ensure_capacity(DynamicBuffer* buffer,size_t capacity){
    if(buffer->capacity<capacity){
        buffer->capacity = max(capacity,2*buffer->capacity);
        buffer->memory= null_check(realloc(buffer,buffer->capacity));
    }
}


// Reading data until all command data is received
static inline int read_command_data(/*sockets*/ DynamicBuffer* buffer, size_t total_size) {
    size_t total_received = 0;

    while (total_received < total_size) {
        // Ensure the buffer has enough space for the remaining data
        ensure_capacity(buffer, total_size);

        // Calculate the remaining bytes to be read
        size_t to_read = total_size - total_received;

        // Read in chunks to avoid overwhelming the socket
        size_t chunk_size = to_read > BUFSIZ ? BUFSIZ : to_read;

        // Temporary buffer to receive data
        char temp_buffer[BUFSIZ];
        size_t received = 0;

        if (tcs_receive(global_tcp_socket, (uint8_t*)temp_buffer, chunk_size, TCS_NO_FLAGS, &received) != TCS_SUCCESS) {
            fprintf(stderr, "Error receiving data from socket.\n");
            return -1;
        }

        // Append received data to the dynamic buffer
        memcpy(buffer->memory + total_received, temp_buffer, received);

        total_received += received;
        buffer->len += received;

        if (received == 0) {
            fprintf(stderr, "Connection closed prematurely.\n");
            return -1;
        }
    }

    return 0; // Success
}

int main(/*sockets*/ int argc, char* argv[]) {
    if(setup(argc,argv)==EXIT_FAILURE){
        return EXIT_FAILURE;
    }

    TaskHeader header = {0};
    DynamicBuffer comand_buffer = {
        null_check(malloc(BUFSIZ)),
        0,
        BUFSIZ
    };

   //  size_t received;
   //  if (tcs_receive(global_tcp_socket, (uint8_t*)&header, sizeof(header), TCS_NO_FLAGS, &received) != TCS_SUCCESS || received != sizeof(header)) {
   //      fprintf(stderr, "Worker %d: Wrong size of TCP message.\n",worker_id);
   //      goto exit_error;
   //  }
   // if(!read_command_data(&comand_buffer,header.packet_size)){
   //      fprintf(stderr, "Worker %d: failed reading TCP message.\n",worker_id);
   //      goto exit_error;

   // }


    // Task example - sending different messages
    // Task Initialization
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_TASK_INIT;
        msg.worker_id = worker_id;
        msg.payload.task_init.task_id = 42;

        size_t udp_sent;
        if (tcs_send_to(global_udp_socket, (uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_TASK_INIT message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_TASK_INIT message.\n", worker_id);
            goto exit_error;
        }
    }

    // Task Done
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_TASK_DONE;
        msg.worker_id = worker_id;
        msg.payload.task_done.status_code = 0;

        size_t udp_sent;
        if (tcs_send_to(global_udp_socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_TASK_DONE message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_TASK_DONE message.\n", worker_id);
            goto exit_error;
        }
    }

    // Update Messages
    for (int i = 0; i < 3; i++) {
        LOG_TO_STDOUT("From Worker %d: output line %d\n", worker_id, i + 1);
        SEND_UPDATE_MESSAGE(global_udp_socket, &server_address, worker_id);
        sleep(0.01);
    }

    // Shutdown Message
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_SHUTDOWN;
        msg.worker_id = worker_id;

        size_t udp_sent;
        if (tcs_send_to(global_udp_socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_SHUTDOWN message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_SHUTDOWN message.\n", worker_id);
            goto exit_error;
        }
    }

    return cleanup();

    exit_error:
    // Shutdown Message
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_CRASH;
        msg.worker_id = worker_id;

        size_t udp_sent;
        if (tcs_send_to(global_udp_socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_SHUTDOWN message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_SHUTDOWN message.\n", worker_id);
        }
    }
    cleanup();
    return EXIT_FAILURE;
}

