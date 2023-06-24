#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024


int main(){
    int server_socket, client_socket, max_socket;
    int activity, val_read;
    int clientSockets[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    struct sockaddr_in server_addr;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1){
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    //bind 
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Binding failed");
        exit(EXIT_FAILURE);
    };

    if(listen(server_socket, MAX_CLIENTS) < 0){
        perror("Listening failed");
        exit(EXIT_FAILURE);
    };

    for(int i = 0; i < MAX_CLIENTS; i++)
        clientSockets[i] = 0;

    while(1){
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(server_socket, &read_fds);
        max_socket = server_socket;

        for(int i = 0; i < MAX_CLIENTS; i++){
            client_socket = clientSockets[i];
            if(client_socket > 0){
                FD_SET(client_socket, &read_fds);
            }

            if(client_socket > max_socket){
                max_socket = client_socket;
            }

        };

        activity = select(max_socket +1, &read_fds, NULL, NULL, NULL);
        if(activity < 0){
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(server_socket, &read_fds)){
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);

            client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
            if(client_socket < 0){
                perror("Accept failed!");
                exit(EXIT_FAILURE);
            }
            for(int i = 0; i < MAX_CLIENTS; i++){
                if(clientSockets[i] == 0){
                    clientSockets[i] = client_socket;
                    break;
                }
            };
        }

        for(int i = 0; i < MAX_CLIENTS; i++){
            client_socket = clientSockets[i];

            if(FD_ISSET(client_socket, &read_fds)){
                val_read = read(client_socket, buffer, BUFFER_SIZE);
                if(val_read == 0){
                    close(client_socket);
                    clientSockets[i] = 0;
                }else{
                    printf("[%d] %.*s", i, val_read, buffer);
                    for(int j = 0; j < MAX_CLIENTS; j++){
                        if(clientSockets[i] != clientSockets[j]){
                            if(clientSockets[j] == 0) continue;
                            buffer[val_read - 1] = '[';
                            buffer[val_read] = i + '0';
                            buffer[val_read + 1] = ']';
                            buffer[val_read + 2] = '\n';
                            buffer[val_read + 3] = '\0';
                            write(clientSockets[j], buffer, val_read + 3);
                        };
                    };
                    //write(client_socket, buffer, val_read);
                };
            }
        };
    }


    return 0;
};
