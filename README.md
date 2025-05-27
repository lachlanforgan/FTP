# FTP Client-Server Implementation

A simple FTP (File Transfer Protocol) client-server implementation written in C.

## Overview

This project consists of a client and server that communicate using the FTP protocol. The implementation supports basic FTP commands like file transfer, directory navigation, and user authentication.

## Features

- User authentication with username and password
- File upload/download capabilities
- Directory navigation and management
- File operations (delete, status)
- Server status information

## Supported Commands

| Command | Description |
|---------|-------------|
| `user <username>` | Send username for authentication |
| `pass <password>` | Send password for authentication |
| `send <filename>` | Upload a file to the server |
| `recv <filename>` | Download a file from the server |
| `mkdir <dir>` | Create a directory on the server |
| `rmdir <dir>` | Remove a directory from the server |
| `ls [dir]` | List directory contents |
| `cd <dir>` | Change working directory |
| `pwd` | Print working directory |
| `dele <file>` | Delete a file on the server |
| `stat [file]` | Display server or file status |
| `help` | Display help message |
| `quit` | Disconnect from the server |

## Technical Details

- Uses TCP sockets for communication
- Implements control connection on port 8041
- Implements data connection on port 8042
- File transfers occur in 100-byte chunks

## Building and Running

### Compilation

```bash
gcc -o forganClientFtp src/forganClientFtp.c
gcc -o forganServerFtp src/forganServerFtp.c
```

### Running the Server

```bash
./forganServerFtp
```

### Running the Client

```bash
./forganClientFtp
```

## Default Users

The server has three pre-configured users:
- Username: `bob`, Password: `123`
- Username: `bruh`, Password: `234`
- Username: `bib`, Password: `987`

## Usage Example

1. Start the server
2. Start the client
3. Login with credentials:
   ```
   my ftp> user bob
   my ftp> pass 123
   ```
4. Execute commands (e.g., `ls`, `pwd`, etc.)
5. Transfer files:
   ```
   my ftp> send myfile.txt  # Upload file
   my ftp> recv serverfile.txt  # Download file
   ```
6. When finished:
   ```
   my ftp> quit
   ```