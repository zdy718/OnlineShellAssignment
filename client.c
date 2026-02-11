#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_COMMAND 1024

/* Function prototypes */
int connect_to_server(const char *host, int port);
void send_command(int socket, const char *command);
void receive_output(int socket);

int main(int argc, char *argv[]) {
    int client_socket;
    char command[MAX_COMMAND];
    char *host;
    int port = PORT;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    
    /* Parse command line arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_host> [port]\n", argv[0]);
        fprintf(stderr, "Example: %s localhost\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 8080\n", argv[0]);
        exit(1);
    }
    
    host = argv[1];
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    
    /* Connect to server */
    client_socket = connect_to_server(host, port);
    if (client_socket < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(1);
    }
    
    /* Get server address information */
    if (getpeername(client_socket, (struct sockaddr *)&server_addr, &addr_len) == 0) {
        printf("Connected to the server (%s:%d) successfully\n",
               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    }
    
    /* Wait for connection confirmation from server */
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    /* Main command loop */
    while (1) {
        /* Display prompt */
        printf("osh> ");
        fflush(stdout);
        
        /* Read command from user */
        if (fgets(command, MAX_COMMAND, stdin) == NULL) {
            break;
        }
        
        /* Remove trailing newline */
        command[strcspn(command, "\n")] = 0;
        
        /* Skip empty commands */
        if (strlen(command) == 0) {
            continue;
        }
        
        /* Check for quit command */
        if (strcmp(command, "quit") == 0) {
            printf("Disconnecting from server...\n");
            send(client_socket, command, strlen(command), 0);
            break;
        }
        
        /* Send command to server */
        send_command(client_socket, command);
        
        /* Receive and display output from server */
        receive_output(client_socket);
    }
    
    close(client_socket);
    printf("Connection closed.\n");
    
    return 0;
}

/*
 * Connect to the remote server
 * Returns socket file descriptor on success, -1 on failure
 */
int connect_to_server(const char *host, int port) {
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *server;
    
    /* Create TCP socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        return -1;
    }
    
    /* Resolve hostname */
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host '%s'\n", host);
        close(sock);
        return -1;
    }
    
    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    server_addr.sin_port = htons(port);
    
    /* Connect to server */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(sock);
        return -1;
    }
    
    return sock;
}

/*
 * Send a command to the server
 */
void send_command(int socket, const char *command) {
    ssize_t bytes_sent;
    
    bytes_sent = send(socket, command, strlen(command), 0);
    if (bytes_sent < 0) {
        perror("Error sending command");
    }
}

/*
 * Receive and display output from the server
 * Continues reading until all output is received
 */
void receive_output(int socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    fd_set read_fds;
    struct timeval timeout;
    int result;
    
    while (1) {
        /* Set up select for non-blocking read with timeout */
        FD_ZERO(&read_fds);
        FD_SET(socket, &read_fds);
        
        /* Set timeout to 100ms */
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        result = select(socket + 1, &read_fds, NULL, NULL, &timeout);
        
        if (result < 0) {
            perror("Error in select");
            break;
        } else if (result == 0) {
            /* Timeout - no more data available */
            break;
        }
        
        /* Data is available to read */
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received < 0) {
            perror("Error receiving data");
            break;
        } else if (bytes_received == 0) {
            /* Server closed connection */
            printf("Server closed connection\n");
            break;
        }
        
        /* Display the received output */
        printf("%s", buffer);
        fflush(stdout);
        
        /* Check if this is the end marker (just a newline) */
        if (bytes_received == 1 && buffer[0] == '\n') {
            break;
        }
    }
}
