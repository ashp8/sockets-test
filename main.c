#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

#define SHA_DIGEST_LENGTH 20
#define BASE64_ENCODED_SIZE (((SHA_DIGEST_LENGTH + 2) / 3) * 4 + 1)



void handle_websocket_client(int client_socket);


int main(int argc, char **argv){
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    int port = 8080;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
       perror("Failed to create socket");
       exit(1);
    };

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Failed to bind");
        exit(1);
    };
    if(listen(server_socket, MAX_CLIENTS) < 0){
        perror("Error in listen!");
        exit(1);
    };
    printf("Server Listening on port %d\n", port);

    client_addr_size = sizeof(client_addr);

    while((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size))){
        printf("New Client Connected!\n");
        //handle some stuff!
        //
        handle_websocket_client(client_socket);

    };

    close(server_socket);
    
    return 0;
};

void handle_websocket_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    bytes_read = read(client_socket, buffer, sizeof(buffer));

    const char* key_header = "Sec-WebSocket-Key: ";
    char* key_start = strstr(buffer, key_header);
    if (key_start == NULL) {
        return;
    }
    key_start += strlen(key_header);
    char* key_end = strchr(key_start, '\r');
    if (key_end == NULL) {
        return;
    }

    char client_key[128];
    size_t key_length = key_end - key_start;
    strncpy(client_key, key_start, key_length);
    client_key[key_length] = '\0';

    const char* magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concat_key[BUFFER_SIZE];
    snprintf(concat_key, sizeof(concat_key), "%s%s", client_key, magic_string);

    // Compute SHA-1 hash of the concatenated key
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)concat_key, strlen(concat_key), sha1_hash);

    // Base64 encode the SHA-1 hash
    char base64_encoded[BUFFER_SIZE];
    EVP_EncodeBlock((unsigned char*)base64_encoded, sha1_hash, SHA_DIGEST_LENGTH);

    // Prepare the WebSocket handshake response
    const char* response_format = "HTTP/1.1 101 Switching Protocols\r\n"
                                  "Upgrade: websocket\r\n"
                                  "Connection: Upgrade\r\n"
                                  "Sec-WebSocket-Accept: %s\r\n"
                                  "\r\n";
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), response_format, base64_encoded);

    // Send WebSocket handshake response to the client
    write(client_socket, response, strlen(response));

    // WebSocket connection established

    // Read and handle WebSocket frames
    while (1) {
        // Read the WebSocket frame header
        bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            // Connection closed or error occurred
            break;
        }

        // Validate the frame header
        unsigned char opcode = buffer[0] & 0x0F;
        unsigned char is_masked = (buffer[1] & 0x80) >> 7;
        unsigned long long payload_length = buffer[1] & 0x7F;

        if (opcode != 0x01 || is_masked) {
            // Invalid frame received, close the connection
            break;
        }

        // Determine the length of the payload
        if (payload_length == 126) {
            // Extended payload length (16-bit)
            if (bytes_read < 4) {
                // Insufficient data, close the connection
                break;
            }
            payload_length = ((buffer[2] & 0xFF) << 8) | (buffer[3] & 0xFF);
            buffer[4] = '\0';  // Null-terminate the payload for simplicity
        } else if (payload_length == 127) {
            // Extended payload length (64-bit)
            // Not implemented in this example
            break;
        } else {
            // Normal payload length
            buffer[2] = '\0';  // Null-terminate the payload for simplicity
        }

        // Handle WebSocket frame data here
        printf("Received message: %s\n", &buffer[2]);


        // For simplicity, let's assume the server echoes back the received frames
        // Write the received WebSocket frame back to the client
        write(client_socket, buffer, bytes_read);
    }
}
