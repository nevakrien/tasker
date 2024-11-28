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

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (tcs_lib_init() != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }

    // Parse arguments
    worker_id_t worker_id = atoi(argv[1]);
    uint16_t tcp_port = (uint16_t)atoi(argv[2]);
    uint16_t udp_port = (uint16_t)atoi(argv[3]);

    TcsSocket tcp_socket, udp_socket;
    tcp_socket = udp_socket = TCS_NULLSOCKET;

    

    // Initialize TCP socket
    if (tcs_create(&tcp_socket, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create TCP socket.\n");
        return EXIT_FAILURE;
    }

    if (tcs_connect(tcp_socket, "127.0.0.1", tcp_port) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to connect to server on TCP port %u.\n", tcp_port);
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    // Send worker ID over TCP
    if (tcs_send(tcp_socket, (const uint8_t*)&worker_id, sizeof(worker_id), TCS_MSG_SENDALL, NULL) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to send worker ID over TCP.\n");
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    DEBUG_LOG( "Worker %d: Sent ID to server via TCP.\n", worker_id);
    LOG_TO_STDOUT("From Worker %d: connected.\n", worker_id);

    // Initialize UDP socket
    if (tcs_create(&udp_socket, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create UDP socket.\n");
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    // Create server address for UDP communication
    struct TcsAddress server_address;
    char address_buffer[256];
    sprintf(address_buffer, "127.0.0.1:%u", udp_port);

    if (tcs_util_string_to_address(address_buffer, &server_address) != TCS_SUCCESS) {
        DEBUG_LOG( "Failed to create server address for UDP communication.\n");
        tcs_destroy(&tcp_socket);
        tcs_destroy(&udp_socket);
        return EXIT_FAILURE;
    }

    // Task example - sending different messages
    // Task Initialization
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_TASK_INIT;
        msg.worker_id = worker_id;
        msg.payload.task_init.task_id = 42;

        size_t udp_sent;
        if (tcs_send_to(udp_socket, (uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_TASK_INIT message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_TASK_INIT message.\n", worker_id);
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
        if (tcs_send_to(udp_socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_TASK_DONE message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_TASK_DONE message.\n", worker_id);
        }
    }

    // Update Messages
    for (int i = 0; i < 3; i++) {
        LOG_TO_STDOUT("From Worker %d: output line %d\n", worker_id, i + 1);
        SEND_UPDATE_MESSAGE(udp_socket, &server_address, worker_id);
        sleep(0.001);
    }

    // Shutdown Message
    {
        WorkerMessage msg;
        msg.magic = WORKER_MESSAGE_MAGIC;
        msg.type = WORKER_MSG_SHUTDOWN;
        msg.worker_id = worker_id;

        size_t udp_sent;
        if (tcs_send_to(udp_socket, (const uint8_t*)&msg, sizeof(msg), TCS_NO_FLAGS, &server_address, &udp_sent) == TCS_SUCCESS && udp_sent == sizeof(msg)) {
            DEBUG_LOG( "Worker %d: Sent WORKER_MSG_SHUTDOWN message.\n", worker_id);
        } else {
            DEBUG_LOG( "Worker %d: Failed to send WORKER_MSG_SHUTDOWN message.\n", worker_id);
        }
    }

    // Cleanup
    tcs_destroy(&tcp_socket);
    tcs_destroy(&udp_socket);

    if (tcs_lib_free() != TCS_SUCCESS) {
        DEBUG_LOG( "Could not free tinycsocket.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

