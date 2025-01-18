#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <unordered_map>

#define PORT 8080
#define MAX_CONNECTIONS 10
#define MAX_QUIZZES 10
#define MAX_PARTICIPANTS 10
#define CODE_LENGTH 4

using namespace std;

void handle_client(int client_fd);
void *client_thread(void *arg);
void create_quiz(int client_fd);
void generate_quiz_code(char* code);

// Global variable to hold quiz codes and their associated ports
unordered_map<string, int> quiz_codes_to_ports;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t thread_id;

    // Initialize server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Main server loop
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }
        cout << "New client connected" << endl;

        // Assign thread for new client
        if (pthread_create(&thread_id, NULL, client_thread, (void *)&new_socket) != 0) {
            perror("Thread creation failed");
        }
        cout << "Thread created" << endl;
    }

    return 0;
}

// Client handling in a separate thread
void *client_thread(void *arg) {
    int client_fd = *((int *)arg);
    char buffer[1024];
    int n;

    // Receive and handle client commands
    while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0';
        
        if (strncmp(buffer, "CREATE", 6) == 0) {
            create_quiz(client_fd);  // Trigger quiz creation and return code
        } else if (strncmp(buffer, "JOIN", 4) == 0) {
            // Join the quiz (validate code and provide corresponding port)
            char code[CODE_LENGTH + 1];
            sscanf(buffer + 5, "%s", code);  // Assuming JOIN <code>
            
            if (quiz_codes_to_ports.find(code) != quiz_codes_to_ports.end()) {
                int quiz_port = quiz_codes_to_ports[code];
                char message[100];
                sprintf(message, "Joining quiz on port %d\n", quiz_port);
                send(client_fd, message, strlen(message), 0);
            } else {
                send(client_fd, "Invalid quiz code\n", 18, 0);
            }
        } else {
            send(client_fd, "Invalid command\n", 16, 0);
        }
    }

    close(client_fd);
    return NULL;
}

// Function to create a quiz on another port and return a unique quiz code
void create_quiz(int client_fd) {
    static int quiz_count = 0;
    if (quiz_count >= MAX_QUIZZES) {
        send(client_fd, "Max quizzes reached\n", 20, 0);
        return;
    }

    // Generate unique quiz code
    char quiz_code[CODE_LENGTH + 1];
    generate_quiz_code(quiz_code);

    // Assign a dynamic port for this quiz
    int quiz_port = PORT + 1 + quiz_count;
    quiz_count++;

    // Store the quiz code and its corresponding port
    quiz_codes_to_ports[quiz_code] = quiz_port;

    // Send the quiz code to the client
    char message[50];
    sprintf(message, "Quiz created! Use this code to join: %s\n", quiz_code);
    send(client_fd, message, strlen(message), 0);
    
    printf("Quiz created with code %s on port %d\n", quiz_code, quiz_port);
}

// Function to generate a random quiz code
void generate_quiz_code(char* code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < CODE_LENGTH; ++i) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[CODE_LENGTH] = '\0';  // Null terminate the string
}
