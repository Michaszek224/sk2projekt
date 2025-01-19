#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <unordered_map>
#include <vector>

#define PORT 8080
#define MAX_CONNECTIONS 10
#define MAX_QUIZZES 10
#define MAX_PARTICIPANTS 10
#define CODE_LENGTH 4

using namespace std;

// Mutex for thread-safe access to shared data
pthread_mutex_t quiz_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to hold user information
struct User {
    int fd;
    string username;
    int score;
};

// Structure to hold quiz data
struct Quiz {
    int port;
    string code;
    vector<string> questions;
    vector<vector<string>> answers;  // Multiple choices per question
    vector<int> correct_answers;
    vector<User> participants;
};

// Global maps to hold quizzes
unordered_map<string, Quiz> active_quizzes;
unordered_map<string, int> quiz_codes_to_ports;

// Function prototypes
void handle_client(int client_fd);
void *client_thread(void *arg);
void create_quiz(int client_fd);
void generate_quiz_code(char* code);
void join_quiz(int client_fd, const char* code);
void send_question_to_participants(const string& quiz_code, int question_index);

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
        pthread_detach(thread_id);  // Detach the thread to free resources automatically
    }

    return 0;
}

void *client_thread(void *arg) {
    int client_fd = *((int *)arg);
    char buffer[1024];
    int n;
    string username;

    // Receive username
    send(client_fd, "Enter your username: ", 21, 0);
    n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return NULL;
    }
    buffer[n] = '\0';
    username = string(buffer);

    cout << "Username: " << username << endl;
    send(client_fd, "Write CREATE to create a quiz or JOIN to join a quiz\n", 54, 0);

    while ((n = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        if (strncmp(buffer, "CREATE", 6) == 0) {
            create_quiz(client_fd);
        } else if (strncmp(buffer, "JOIN", 4) == 0) {
            char code[CODE_LENGTH + 1];
            sscanf(buffer + 5, "%s", code);  // Extract quiz code
            join_quiz(client_fd, code);
        } else {
            send(client_fd, "Invalid command\n", 16, 0);
        }
    }

    close(client_fd);
    return NULL;
}

void create_quiz(int client_fd) {
    static int quiz_count = 0;
    if (quiz_count >= MAX_QUIZZES) {
        send(client_fd, "Max quizzes reached\n", 20, 0);
        return;
    }

    // Generate unique quiz code
    char quiz_code[CODE_LENGTH + 1];
    generate_quiz_code(quiz_code);

    pthread_mutex_lock(&quiz_mutex);

    // Create and populate a new quiz
    Quiz new_quiz;
    new_quiz.port = PORT + 1 + quiz_count;
    new_quiz.code = quiz_code;
    new_quiz.questions = {"What is 2 + 2?", "What is the capital of France?"};
    new_quiz.answers = {{"1. 3", "2. 4", "3. 5"}, {"1. Paris", "2. London", "3. Berlin"}};
    new_quiz.correct_answers = {1, 0};  // Index of correct answers

    active_quizzes[quiz_code] = new_quiz;
    quiz_codes_to_ports[quiz_code] = new_quiz.port;
    quiz_count++;

    pthread_mutex_unlock(&quiz_mutex);

    char message[100];
    sprintf(message, "Quiz created! Use this code to join: %s\n", quiz_code);
    send(client_fd, message, strlen(message), 0);

    printf("Quiz created with code %s on port %d\n", quiz_code, new_quiz.port);
}

void generate_quiz_code(char* code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < CODE_LENGTH; ++i) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[CODE_LENGTH] = '\0';  // Null terminate the string
}

void join_quiz(int client_fd, const char* code) {
    pthread_mutex_lock(&quiz_mutex);
    auto it = active_quizzes.find(code);
    if (it == active_quizzes.end()) {
        send(client_fd, "Invalid quiz code\n", 18, 0);
        pthread_mutex_unlock(&quiz_mutex);
        return;
    }

    Quiz& quiz = it->second;
    User new_user = {client_fd, "Anonymous", 0};
    quiz.participants.push_back(new_user);

    pthread_mutex_unlock(&quiz_mutex);

    send(client_fd, "You joined the quiz!\n", 21, 0);
    send_question_to_participants(code, 0);
}

void send_question_to_participants(const string& quiz_code, int question_index) {
    pthread_mutex_lock(&quiz_mutex);
    auto it = active_quizzes.find(quiz_code);
    if (it == active_quizzes.end() || question_index >= it->second.questions.size()) {
        pthread_mutex_unlock(&quiz_mutex);
        return;
    }

    Quiz& quiz = it->second;
    string question = quiz.questions[question_index];
    string answers = "";
    for (const auto& ans : quiz.answers[question_index]) {
        answers += ans + "\n";
    }

    for (auto& participant : quiz.participants) {
        send(participant.fd, question.c_str(), question.size(), 0);
        send(participant.fd, answers.c_str(), answers.size(), 0);
    }

    pthread_mutex_unlock(&quiz_mutex);
}
