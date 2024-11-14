#include "http-server.h"

void start_server(void(*handler)(char*, int), int port) {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    
    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Enable reusing address
    int enable = 1; 
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)  
        perror("setsockopt(SO_REUSEADDR) failed");                                

    // Defining server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Binding server socket to address  
    socklen_t addr_len = sizeof(server_address);
    if (bind(server_socket, (struct sockaddr *)&server_address, addr_len) < 0) {
        perror("bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections with backlog of 10
    // I think this means it can queue up 10 pending connections
    if (listen(server_socket, 10) < 0) {
        perror("listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Get port number
    if (getsockname(server_socket, (struct sockaddr *)&server_address, &addr_len) == -1) {
        perror("getsockname failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", ntohs(server_address.sin_port));
    char buffer[BUFFER_SIZE];
    // Main server loop
    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len)) < 0) {
            perror("accept failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Receive the request
        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if(bytes < 0) {printf("error receiving\n");close(client_socket);continue;}
        buffer[bytes] = '\0';  

        (*handler)(buffer, client_socket);

        // Close the connection with the client
        close(client_socket);
    }

    close(server_socket);
}