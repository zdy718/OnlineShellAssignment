# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = 

# Target executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Default target - build both server and client
all: $(SERVER) $(CLIENT)

# Build server
$(SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJ) $(LDFLAGS)
	@echo "Server built successfully"

# Build client
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_OBJ) $(LDFLAGS)
	@echo "Client built successfully"

# Compile server object file
$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

# Compile client object file
$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

# Clean build artifacts
clean:
	rm -f $(SERVER) $(CLIENT) $(SERVER_OBJ) $(CLIENT_OBJ)
	@echo "Cleaned build artifacts"

# Run server (for testing)
run-server: $(SERVER)
	./$(SERVER)

# Run client (for testing - requires hostname as argument)
run-client: $(CLIENT)
	@echo "Usage: ./$(CLIENT) <hostname> [port]"
	@echo "Example: ./$(CLIENT) localhost"

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build both server and client"
	@echo "  make server   - Build only the server"
	@echo "  make client   - Build only the client"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run-server - Build and run the server"
	@echo "  make run-client - Show client usage"
	@echo "  make help     - Show this help message"

.PHONY: all clean run-server run-client help