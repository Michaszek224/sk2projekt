#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>

#define PORT 8080
#define MAX_CONNECTIONS 10
#define MAX_QUIZZES 10
#define MAX_PARTICIPANTS 10

using namespace std;

void handle_client(int client_fd);
void *client_thread(void *arg);
void create_quiz(int client_fd);

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
            create_quiz(client_fd);  // Trigger quiz creation on a new port
        } else if (strncmp(buffer, "JOIN", 4) == 0) {
            // Handle joining a quiz logic (e.g. validate quiz code)
            send(client_fd, "Joined quiz\n", 12, 0);
            cout << "Joining quiz" << endl;
        } else {
            send(client_fd, "Invalid command\n", 16, 0);
        }
    }

    close(client_fd);
    return NULL;
}

// Function to create a quiz on another port
void create_quiz(int client_fd) {
    static int quiz_count = 0;
    if (quiz_count >= MAX_QUIZZES) {
        send(client_fd, "Max quizzes reached\n", 20, 0);
        return;
    }

    int quiz_port = PORT + 1 + quiz_count;  // Assign a unique port number for each quiz
    quiz_count++;

    // Set up a new server for the quiz
    int quiz_server_fd;
    struct sockaddr_in quiz_server_addr;

    if ((quiz_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Quiz socket failed");
        return;
    }

    quiz_server_addr.sin_family = AF_INET;
    quiz_server_addr.sin_addr.s_addr = INADDR_ANY;
    quiz_server_addr.sin_port = htons(quiz_port);

    if (bind(quiz_server_fd, (struct sockaddr *)&quiz_server_addr, sizeof(quiz_server_addr)) < 0) {
        perror("Quiz Bind failed");
        return;
    }

    if (listen(quiz_server_fd, MAX_PARTICIPANTS) < 0) {
        perror("Quiz Listen failed");
        return;
    }

    printf("Quiz created on port %d\n", quiz_port);

    // Send the quiz port to the client for them to join
    char message[50];
    sprintf(message, "Quiz created! You can join on port %d\n", quiz_port);
    send(client_fd, message, strlen(message), 0);
}
