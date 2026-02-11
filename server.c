#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define MAX_ARGS 64

/* Function prototypes */
void handle_client(int client_socket, struct sockaddr_in client_addr);
void execute_command(int client_socket, char *command);
void parse_command(char *command, char **args);
void sigchld_handler(int sig);

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pid_t pid;
    int port = PORT;
    
    /* Allow custom port from command line */
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    /* Set up signal handler to prevent zombie processes */
    signal(SIGCHLD, sigchld_handler);
    
    /* Create TCP socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }
    
    /* Allow socket reuse to avoid "Address already in use" error */
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error setting socket options");
        exit(1);
    }
    
    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind socket to address */
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }
    
    /* Listen for connections */
    if (listen(server_socket, 5) < 0) {
        perror("Error listening on socket");
        exit(1);
    }
    
    printf("Server listening on port %d...\n", port);
    
    /* Main server loop - accept and handle connections */
    while (1) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Error accepting connection");
            continue;
        }
        
        /* Print connection message */
        printf("Client (%s:%d) Connected to the server successfully\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Fork child process to handle this client */
        pid = fork();
        
        if (pid < 0) {
            perror("Error forking process");
            close(client_socket);
            continue;
        }
        
        if (pid == 0) {
            /* Child process */
            close(server_socket);  /* Child doesn't need the listening socket */
            handle_client(client_socket, client_addr);
            exit(0);
        } else {
            /* Parent process */
            close(client_socket);  /* Parent doesn't need this socket */
        }
    }
    
    close(server_socket);
    return 0;
}

/*
 * Handle communication with a single client
 * Receives commands and executes them
 */
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    /* Send connection success message to client */
    const char *success_msg = "CONNECTED\n";
    send(client_socket, success_msg, strlen(success_msg), 0);
    
    /* Process commands from client */
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            /* Client disconnected or error occurred */
            if (bytes_received == 0) {
                printf("Client (%s:%d) disconnected\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            } else {
                perror("Error receiving data");
            }
            break;
        }
        
        /* Remove trailing newline */
        buffer[strcspn(buffer, "\n")] = 0;
        
        /* Check for quit command */
        if (strcmp(buffer, "quit") == 0) {
            printf("Client (%s:%d) sent quit command\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        }
        
        /* Execute the command */
        execute_command(client_socket, buffer);
    }
    
    close(client_socket);
}

/*
 * Execute a shell command and send output back to client
 * Uses fork() and execvp() to run the command
 * Redirects stdout and stderr to the client socket
 */
void execute_command(int client_socket, char *command) {
    char *args[MAX_ARGS];
    int pipefd[2];
    pid_t pid;
    char output[BUFFER_SIZE];
    ssize_t bytes_read;
    
    /* Create pipe for command output */
    if (pipe(pipefd) < 0) {
        perror("Error creating pipe");
        const char *error_msg = "Error: Could not create pipe\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }
    
    /* Fork to execute command */
    pid = fork();
    
    if (pid < 0) {
        perror("Error forking for command execution");
        const char *error_msg = "Error: Could not fork process\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    
    if (pid == 0) {
        /* Child process - execute the command */
        close(pipefd[0]);  /* Close read end */
        
        /* Redirect stdout and stderr to pipe */
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        /* Parse command into arguments */
        parse_command(command, args);
        
        /* Execute command */
        execvp(args[0], args);
        
        /* If execvp returns, an error occurred */
        fprintf(stderr, "Error: Command '%s' not found or failed to execute\n", args[0]);
        exit(1);
    } else {
        /* Parent process - read command output and send to client */
        close(pipefd[1]);  /* Close write end */
        
        /* Read all output from pipe and send to client */
        while ((bytes_read = read(pipefd[0], output, BUFFER_SIZE - 1)) > 0) {
            send(client_socket, output, bytes_read, 0);
        }
        
        close(pipefd[0]);
        
        /* Wait for child to finish */
        waitpid(pid, NULL, 0);
        
        /* Send end-of-output marker */
        const char *end_marker = "\n";
        send(client_socket, end_marker, strlen(end_marker), 0);
    }
}

/*
 * Parse a command string into an array of arguments
 * Example: "ls -la /tmp" becomes ["ls", "-la", "/tmp", NULL]
 */
void parse_command(char *command, char **args) {
    int i = 0;
    char *token;
    
    /* Tokenize command by spaces */
    token = strtok(command, " \t");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL;  /* NULL-terminate the array */
}

/*
 * Signal handler for SIGCHLD
 * Prevents zombie processes by reaping terminated children
 */
void sigchld_handler(int sig) {
    (void)sig;  /* Unused parameter */
    
    /* Reap all terminated child processes */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}
