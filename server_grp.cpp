#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

std::map<int, std::thread> client_threads;
std::map<std::string, std::string> users;
std::map<std::string, int> user_socket;
std::map<int, std::string> socket_user;
std::map<std::string, std::set<int>> user_groups;

std::mutex user_mutex, group_mutex;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void broadcast(const int client_socket, std::string message) {
    message = "[ Server ]: " + message;
    user_mutex.lock();
    for (auto & it : user_socket) {
        if (it.second != client_socket) {
            send(it.second, message.c_str(), BUFFER_SIZE, 0);
        }
    }
    user_mutex.unlock();
}

std::string trim(const std::string &str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos) {
        return ""; // Return empty string if only whitespace is found
    }

    return str.substr(start, end - start + 1);
}

void authenticate(int client_socket) {
    char buffer[BUFFER_SIZE];

    // Request username and password
    memset(buffer, 0, sizeof(buffer));
    send(client_socket, "Enter username: ", 100, 0);
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) { // Handle client disconnection during authentication
        close(client_socket);
        return;
    }
    std::string username = trim(std::string(buffer));

    memset(buffer, 0, sizeof(buffer));
    send(client_socket, "Enter password: ", 100, 0);
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) { // Handle client disconnection during authentication
        close(client_socket);
        return;
    }
    std::string password = std::string(buffer);  // No trimming applied

    // Authentication check (no empty string checks for username or password)
    user_mutex.lock();
    if (users.find(username) == users.end() || users[username] != password) {
        user_mutex.unlock();
        send(client_socket, "Authentication failed.", 100, 0);
        close(client_socket); // Close the connection for failed clients
        return;
    } else {
        user_socket[username] = client_socket;
        socket_user[client_socket] = username;
        user_mutex.unlock();
        send(client_socket, "Welcome to the chat server!", 100, 0);

        // Notify other clients that the new user has joined the chat
        broadcast(client_socket, username + " has joined the chat.");
    }
}

void msg(int client_socket, const std::string& receiver, std::string message) {
    user_mutex.lock();
    if (user_socket.find(receiver) == user_socket.end()) {
        user_mutex.unlock();
        send(client_socket, "User not found.", 100, 0);
    } else {
        message = "[ " + socket_user[client_socket] + " ]: " + message;
        user_mutex.unlock();
        send(user_socket[receiver], message.c_str(), BUFFER_SIZE, 0);
    }
}

void create_group(int client_socket, const std::string& group_name) {
    group_mutex.lock();
    if (user_groups.find(group_name) != user_groups.end()) {
        group_mutex.unlock();
        send(client_socket, "Group already exists.", 100, 0);
        return;
    }
    user_groups[group_name].insert(client_socket);
    group_mutex.unlock();
    send(client_socket, ("Group " + group_name + " created.").c_str(), 100, 0);
}

void join_group(int client_socket, const std::string& group_name) {
    group_mutex.lock();
    if (user_groups.find(group_name) == user_groups.end()) {
        group_mutex.unlock();
        send(client_socket, "Group does not exist.", 100, 0);
        return;
    }

    if (user_groups[group_name].find(client_socket) != user_groups[group_name].end()) {
        group_mutex.unlock();
        send(client_socket, "You are already in this group.", 100, 0);
        return;
    }

    user_groups[group_name].insert(client_socket);
    group_mutex.unlock();
    send(client_socket, ("You joined the group " + group_name + ".").c_str(), 100, 0);
}

void leave_group(int client_socket, const std::string& group_name) {
    group_mutex.lock();
    if (user_groups[group_name].find(client_socket) == user_groups[group_name].end()) {
        group_mutex.unlock();
        send(client_socket, "You are not part of this group.", 100, 0);
        return;
    }
    user_groups[group_name].erase(client_socket);
    group_mutex.unlock();
    send(client_socket, ("You have left the group " + group_name + ".").c_str(), 100, 0);
}

void group_msg(int client_socket, const std::string& group_name, std::string message) {
    group_mutex.lock();
    if (user_groups[group_name].find(client_socket) == user_groups[group_name].end()) {
        group_mutex.unlock();
        send(client_socket, ("You are not a member of group: " + group_name).c_str(), 100, 0);
        return;
    }
    message = "[ Group " + group_name + " ]: " + message;
    for (auto it = user_groups[group_name].begin(); it != user_groups[group_name].end(); it++) {
        if (*it != client_socket) {
            send(*it, message.c_str(), BUFFER_SIZE, 0);
        } else {
            send(*it, "Message successfully sent", 100, 0);
        }
    }
    group_mutex.unlock();
}

void handle_client(int client_socket) {
    std::cout << "Client socket " << client_socket << std::endl;
    authenticate(client_socket);
    if (client_socket < 0) return;  // Ensure the client is authenticated before continuing

    while (true) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected: " << client_socket << std::endl;
            // Handle client disconnection gracefully
            user_mutex.lock();
            std::string username = socket_user[client_socket];
            socket_user.erase(client_socket);
            user_socket.erase(username);
            user_mutex.unlock();

            // Remove the client from all groups they were part of
            group_mutex.lock();
            for (auto& group : user_groups) {
                group.second.erase(client_socket);
            }
            group_mutex.unlock();

            close(client_socket);
            return;
        }

        std::string input(buffer);
        std::stringstream ss(input);
        std::string command;
        ss >> command;
        if (command == "/broadcast") {
            std::string message;
            getline(ss, message);
            broadcast(client_socket, message);
        } else if (command == "/msg") {
            std::string receiver, message;
            ss >> receiver;
            getline(ss, message);
            msg(client_socket, receiver, message);
        } else if (command == "/create_group") {
            std::string group_name;
            ss >> group_name;
            create_group(client_socket, group_name);
        } else if (command == "/join_group") {
            std::string group_name;
            ss >> group_name;
            join_group(client_socket, group_name);
        } else if (command == "/leave_group") {
            std::string group_name;
            ss >> group_name;
            leave_group(client_socket, group_name);
        } else if (command == "/group_msg") {
            std::string group_name, message;
            ss >> group_name;
            getline(ss, message);
            group_msg(client_socket, group_name, message);
        } else if (command == "/exit") {
            break;
        } else {
            send(client_socket, "Invalid command", 100, 0);
        }
    }
}

void load_all_users() {
    std::ifstream file("users.txt");
    std::string line;
    while (getline(file, line)) {
        uint32_t del_pos = line.find(":");
        std::string username = line.substr(0, del_pos);
        std::string password = line.substr(del_pos + 1);
        users[username] = password;
    }
    file.close();
}

void start_server() {
    load_all_users();

    int server_socket, client_socket;
    socklen_t clilen;
    struct sockaddr_in server_address, client_address;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        error("ERROR opening socket");
    }

    memset((char *)&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(12345);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (::bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        error("ERROR in Binding");
    }

    if (listen(server_socket, 10) < 0) {
        error("ERROR in listening");
    }

    std::cout << "Server Listening on " << ntohs(server_address.sin_port) << std::endl;

    while (true) {
        clilen = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &clilen);
        if (client_socket < 0) {
            error("ERROR in accepting request");
            continue;
        }
        client_threads[client_socket] = std::thread(handle_client, client_socket);
        client_threads[client_socket].detach();
    }
    close(server_socket);
}

int main() {
    start_server();
    return 0;
}