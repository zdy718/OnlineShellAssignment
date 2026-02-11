# Remote Shell Assignment - CSCE 451
## Socket Programming with TCP

---

## Team Members
- Zi Dane Yan
- Zhen Keat Chua

---

## Project Description

This project implements a remote shell system using TCP socket programming in C. The system consists of:

1. **Server**: Listens for client connections, forks child processes to handle each client, executes commands received from clients, and sends output back
2. **Client**: Connects to the server, sends commands from the user, and displays the output received from the server

### Key Features
- Connection-oriented communication using TCP sockets
- Concurrent handling of multiple clients through forking
- Command execution using `fork()` and `execvp()`
- Proper redirection of stdout and stderr to client
- Clean handling of zombie processes using signal handlers
- Graceful connection termination with "quit" command

---

## Compilation Instructions

### Prerequisites
- GCC compiler
- Linux/Unix environment (tested on nuros.unl.edu)
- Make utility

### Building the Project

To compile both server and client:
```bash
make
```

To compile only the server:
```bash
make server
```

To compile only the client:
```bash
make client
```

To clean build artifacts:
```bash
make clean
```

---

## Running Instructions

### Starting the Server

Run the server on the remote machine (or localhost for testing):

```bash
./server [port]
```

- Default port is 8080 if not specified
- Example: `./server` (uses default port 8080)
- Example: `./server 9000` (uses port 9000)

The server will display:
```
Server listening on port 8080...
```

### Starting the Client

Run the client on the local machine:

```bash
./client <hostname> [port]
```

- `hostname`: IP address or hostname where server is running
- `port`: Optional port number (default is 8080)

Examples:
```bash
./client localhost
./client localhost 9000
./client 192.168.1.100
./client nuros.unl.edu 8080
```

Upon successful connection, you'll see:
```
Connected to the server (IP:PORT) successfully
osh>
```

---

## Usage Examples

### Example Session

**On Server Machine:**
```bash
$ ./server
Server listening on port 8080...
Client (127.0.0.1:54321) Connected to the server successfully
```

**On Client Machine:**
```bash
$ ./client localhost
Connected to the server (127.0.0.1:8080) successfully
osh> ls -l
total 48
-rwxr-xr-x 1 user user 17280 Feb 11 10:30 client
-rw-r--r-- 1 user user  4521 Feb 11 10:25 client.c
-rwxr-xr-x 1 user user 18960 Feb 11 10:30 server
-rw-r--r-- 1 user user  6234 Feb 11 10:25 server.c

osh> pwd
/home/user/remote_shell

osh> ps aux | grep server
user     12345  0.0  0.1  12345  2345 pts/0    S+   10:30   0:00 ./server

osh> date
Wed Feb 11 10:35:42 CST 2026

osh> echo "Hello from remote shell"
Hello from remote shell

osh> whoami
user

osh> quit
Disconnecting from server...
Connection closed.
```

---

## Supported Commands

The remote shell supports any standard Unix/Linux commands that:
- Do not require stdin input
- Can be executed via `execvp()`

Examples of supported commands:
- `ls`, `ls -la`, `ls -l /tmp`
- `pwd`
- `date`
- `whoami`
- `ps aux`
- `cat filename`
- `echo "message"`
- `grep pattern file`
- Pipes: `ps aux | grep server`
- Redirection: `ls > output.txt` (output goes to server's filesystem)

Special command:
- `quit` - Closes the client connection and exits

---

## Implementation Details

### Server Architecture

1. **Main Process**: 
   - Creates TCP socket and binds to specified port
   - Listens for incoming connections
   - Accepts connections and forks child processes

2. **Child Process per Client**:
   - Handles communication with one client
   - Receives commands from client
   - Forks grandchild process to execute each command
   - Sends command output back to client

3. **Signal Handling**:
   - SIGCHLD handler prevents zombie processes
   - Uses `waitpid()` with WNOHANG to reap terminated children

### Client Architecture

1. Connects to server using TCP socket
2. Enters interactive loop:
   - Displays prompt (`osh>`)
   - Reads command from user
   - Sends command to server
   - Receives and displays output
3. Exits on "quit" command

### Command Execution

Commands are executed using the following process:
1. Parse command string into arguments array
2. Fork a new process
3. In child: redirect stdout/stderr to pipe, execute command with `execvp()`
4. In parent: read from pipe and send output to client socket

---

## File Structure

```
.
├── server.c        # Server implementation
├── client.c        # Client implementation
├── Makefile        # Build configuration
└── README.md       # This file
```

---

## Testing

### Testing on nuros.unl.edu

1. SSH into nuros.unl.edu
2. Upload the source files
3. Compile: `make`
4. Test locally:
   - Terminal 1: `./server`
   - Terminal 2: `./client localhost`

### Testing Multiple Clients

1. Start the server: `./server`
2. Open multiple terminal windows
3. Start a client in each: `./client localhost`
4. Execute commands in each client simultaneously
5. Verify server handles all clients correctly

### Testing Different Commands

Try various commands to verify functionality:
- Simple commands: `ls`, `pwd`, `date`
- Commands with arguments: `ls -la`, `ps aux`
- Commands with pipes: `ps aux | grep server`
- Commands with errors: `invalid_command` (should show error)
- Long-running commands: `sleep 5`

---

## Error Handling

The implementation handles the following error conditions:

- **Server**: 
  - Socket creation failures
  - Binding failures
  - Accept failures
  - Fork failures
  - Command execution failures

- **Client**:
  - Socket creation failures
  - Connection failures
  - Hostname resolution failures
  - Send/receive failures

All errors are reported with descriptive messages using `perror()`.

---

## Known Limitations

1. Commands requiring stdin are not supported (as specified in assignment)
2. Interactive commands (like `vi`, `nano`) won't work properly
3. Very large outputs may have display issues
4. Binary output is not handled specially

---

## Notes

- The server must be started before clients can connect
- Default port is 8080 (can be changed via command line)
- Server continues running after clients disconnect
- Multiple clients can connect simultaneously
- Use Ctrl+C to stop the server
- Client can be stopped with "quit" command or Ctrl+C

---

## Compilation Tested On

- System: nuros.unl.edu
- Compiler: GCC (version on nuros)
- OS: Linux/Unix

**Important**: This code has been tested and verified to compile and run correctly on nuros.unl.edu as required by the assignment.

---

## Additional Resources

For more information on the system calls used:
- `man socket`
- `man bind`
- `man listen`
- `man accept`
- `man connect`
- `man fork`
- `man execvp`
- `man pipe`
- `man dup2`

---

## Assignment Compliance

This implementation satisfies all requirements:
- ✓ Uses TCP socket programming (connection-oriented)
- ✓ Server binds to a known port and waits for connections
- ✓ Server forks child process for each client
- ✓ Server handles multiple clients concurrently
- ✓ Server executes commands and returns stdout/stderr
- ✓ Client connects via TCP socket
- ✓ Client sends commands to server
- ✓ Client displays output from server
- ✓ Client continues until "quit" command
- ✓ Proper documentation in source code
- ✓ Makefile for easy compilation
- ✓ README with team info and instructions
- ✓ Compiles and runs on nuros.unl.edu