# README.md

## Assignment Features Implemented

- **Basic Server Functionality**: The server listens on a TCP port (12345) and accepts multiple concurrent client connections.
- **User Authentication**: Users are authenticated using a `users.txt` file that stores usernames and passwords. Authentication fails if incorrect credentials are provided.
- **Messaging Features**:
    - **Broadcast messages** to all connected clients.
    - **Private messages** to specific users.
    - **Group messaging** for communication within specific groups.
- **Group Management**:
    - Create groups.
    - Join or leave groups.
    - Send group messages.
- **Concurrency**: Each client connection is handled by a separate thread to ensure the server can manage multiple clients simultaneously.

## Features Not Implemented

- **User Session Management**: The server doesn’t store long-term session data, so if the server crashes, all users will need to log in again.

## Design Decisions

### 1. **Thread vs Process**:
- **Decision**: Threads were chosen over processes for each client connection.
- **Reason**: Threads are lightweight and share the same memory space, making them more efficient for this application. Each client is handled in its own thread to allow concurrent processing of multiple requests, without the overhead of process management.

### 2. **Use of Mutexes for Synchronization**:
- **Decision**: Mutexes (`std::mutex`) were used for synchronizing shared resources (such as `user_socket`, `socket_user`, and `user_groups`).
- **Reason**: These shared resources need protection from concurrent access by multiple threads to avoid data races. Using a mutex ensures thread safety when modifying or accessing shared data.

### 3. **Group Management**:
- **Decision**: Groups were implemented using a `std::map` with group names as keys and sets of client sockets as values.
- **Reason**: This structure allows quick lookups to check group membership and efficient insertion/removal of members. The use of sets ensures that each user can only be a member of a group once.

### 4. **Message Handling**:
- **Decision**: Each command sent by clients is parsed and processed using a string parsing approach.
- **Reason**: It is simple and effective for this application where commands are easily distinguishable based on their prefixes (e.g., `/broadcast`, `/msg`, etc.).

## Implementation

### High-Level Idea of Important Functions

1. **`authenticate(int client_socket)`**:
    - Handles the authentication process by prompting the client for a username and password.
    - Checks the provided credentials against the `users.txt` file and either grants or denies access.

2. **`broadcast(int client_socket, std::string message)`**:
    - Sends a broadcast message to all connected clients except the sender.
    - Ensures all clients receive the same message.

3. **`msg(int client_socket, const std::string& receiver, std::string message)`**:
    - Sends a private message to a specific user.
    - Checks if the user exists before attempting to send the message.

4. **`create_group(int client_socket, const std::string& group_name)`**:
    - Creates a new group if it does not already exist.
    - Adds the client to the new group.

5. **`group_msg(int client_socket, const std::string& group_name, std::string message)`**:
    - Sends a message to all members of a specified group.
    - Only users who have joined the group can send and receive messages.

### Code Flow

The server starts by loading the list of users from `users.txt` and then listens for incoming client connections. When a new client connects, the server spawns a new thread to handle that client. The client is authenticated, and once authenticated, the client can issue commands such as broadcasting messages, sending private messages, creating or joining groups, etc.
 

## Testing

### Correctness Testing
- **Client Authentication**: Clients with valid credentials are allowed to connect, and others are disconnected with an authentication failure message.
- **Messaging**: Tested sending broadcast, private messages, and group messages to ensure correct delivery.
- **Group Management**: Ensured that users can create groups, join groups, and send group messages.

### Stress Testing
- **Multiple Clients**: The server was tested with multiple clients connecting simultaneously and performing various actions (broadcasting messages, creating groups, etc.).
- **High Message Volume**: The server was tested with large volumes of messages to ensure stability under load.

## Restrictions in Your Server

- **Maximum Clients**: The maximum number of clients is dependent on the available system resources (e.g., memory, socket limits).
- **Maximum Groups**: The number of groups is only limited by memory and system capacity.
- **Group Size**: No explicit limit on group size, but each group is managed via a set of client sockets, so it's limited by system resources.
- **Message Size**: Messages are limited to `BUFFER_SIZE` (1024 bytes).

## Challenges

1. **Handling Client Disconnects**: Managing client disconnects gracefully was a challenge. When a client disconnects, all resources related to that client (socket, group membership, etc.) need to be cleaned up.
2. **Concurrency Issues**: Ensuring thread safety while accessing shared data structures like `user_socket` and `user_groups` required careful synchronization using mutexes.
3. **Server Stopping on Failed Authentication**: If one client fails authentication, the server was stopping, and all other clients would be disconnected. This issue arises from improper error handling, where a failed authentication might cause an exit or shutdown of the server.
   Fix: Ensure that after authentication failure, only the client that failed is disconnected, and the server continues accepting other clients.
## Contribution of Each Member

- **Deepika Sahu**: 33% - Developed server initialization, connection handling, and user authentication. Wrote unit tests, handled debugging.
- **Satyam Gupta**: 34% - Designed and implemented the core functionality of messaging systems (private and group) and group creation/management features.
- **Divyansh Verma**: 33% - Implemented thread management and concurrency handling, including synchronization using mutexes. Prepared the README file.

## Sources Referred

- C++ documentation: [cppreference.com](https://en.cppreference.com/)
- Socket Programming: [Socket Programming Guide](https://beej.us/guide/bgnet/)
- Threading and synchronization: [Modern C++ Threading - cppcon 2019](https://www.youtube.com/watch?v=NKHG1lJjCzk)

## Declaration

We hereby declare that the code submitted is our original work and has not been plagiarized. 

## Feedback

- **Suggestions**: It could benefit from additional documentation and examples in the project’s repository.
- **Feedback**: It was a great exercise to work on real-world server-client communication with multi-threading.

