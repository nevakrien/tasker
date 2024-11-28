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

#define LOG_TO_STDOUT(format, ...)                                   \
    do {                                                            \
        char temp_buffer[1024];                                     \
        int bytes = snprintf(temp_buffer, sizeof(temp_buffer), format, ##__VA_ARGS__); \
        if (bytes > 0) {                                            \
            printf("%s", temp_buffer);                              \
            total_bytes_written += (uint64_t)bytes;                 \
        }                                                           \
    } while (0)

void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s <worker_id> <tcp_port> <udp_port>\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (tcs_lib_init() != TCS_SUCCESS) {
        fprintf(stderr, "Failed to initialize tinycsocket.\n");
        return EXIT_FAILURE;
    }

    // Parse arguments
    int worker_id = atoi(argv[1]);
    uint16_t tcp_port = (uint16_t)atoi(argv[2]);
    uint16_t udp_port = (uint16_t)atoi(argv[3]);

    TcsSocket tcp_socket, udp_socket;
    tcp_socket=udp_socket=TCS_NULLSOCKET;
    
    uint64_t total_bytes_written = 0;

    // Initialize TCP socket
    if (tcs_create(&tcp_socket, TCS_TYPE_TCP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create TCP socket.\n");
        return EXIT_FAILURE;
    }

    if (tcs_connect(tcp_socket, "127.0.0.1", tcp_port) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to connect to server on TCP port %u.\n", tcp_port);
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    // Send worker ID over TCP
    uint32_t id_message = (uint32_t)worker_id;
    if (tcs_send(tcp_socket, (const uint8_t*)&id_message, sizeof(id_message), TCS_MSG_SENDALL, NULL) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to send worker ID over TCP.\n");
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Worker %d: Sent ID to server via TCP.\n", worker_id);
    LOG_TO_STDOUT("Worker %d connected.\n", worker_id);

    // Initialize UDP socket
    if (tcs_create(&udp_socket, TCS_TYPE_UDP_IP4) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create UDP socket.\n");
        tcs_destroy(&tcp_socket);
        return EXIT_FAILURE;
    }

    // Create server address for UDP communication
    struct TcsAddress server_address;
    char address_buffer[256];
    sprintf(address_buffer, "127.0.0.1:%u", udp_port);

    if (tcs_util_string_to_address(address_buffer, &server_address) != TCS_SUCCESS) {
        fprintf(stderr, "Failed to create server address for UDP communication.\n");
        tcs_destroy(&tcp_socket);
        tcs_destroy(&udp_socket);
        return EXIT_FAILURE;
    }

    // Periodically send UDP messages
    for (int i = 0; i < 3; i++) {
        LOG_TO_STDOUT("Worker %d ping at %ld.\n", worker_id, time(NULL));

        WorkerUdpMessage udp_message;
        udp_message.worker_id = (uint32_t)worker_id;
        udp_message.bytes_written = total_bytes_written;

        size_t udp_sent;
        if (tcs_send_to(udp_socket, (const uint8_t*)&udp_message, sizeof(udp_message), TCS_NO_FLAGS, &server_address, &udp_sent) != TCS_SUCCESS || udp_sent != sizeof(udp_message)) {
            fprintf(stderr, "Worker %d: Failed to send UDP message.\n", worker_id);
        } else {
            fprintf(stderr, "Worker %u: Sent UDP message (bytes_written: %" PRIu64 ").\n", worker_id, total_bytes_written);
        }

        total_bytes_written = 0;
        sleep(1);
    }

    // Cleanup
    tcs_destroy(&tcp_socket);
    tcs_destroy(&udp_socket);

    if (tcs_lib_free() != TCS_SUCCESS) {
        fprintf(stderr, "Could not free tinycsocket.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
