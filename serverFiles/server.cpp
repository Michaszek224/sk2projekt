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

// Global map to hold quiz codes to their associated ports and quizzes
unordered_map<string, Quiz> active_quizzes;
unordered_map<string, int> quiz_codes_to_ports;

void handle_client(int client_fd);
void *client_thread(void *arg);
void create_quiz(int client_fd);
void generate_quiz_code(char* code);
void join_quiz(int client_fd, const char* code);
void send_question_to_participants(const string& quiz_code, int question_index);
void handle_answer(int client_fd, int quiz_port, int question_index, int answer);

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
    char username[50];
    
    // Receive and handle client commands
    send(client_fd, "Enter your username: ", 21, 0);
    n = recv(client_fd, username, sizeof(username), 0);
    username[n - 1] = '\0';  // Remove newline character
    
    cout << "Username: " << username << endl;

    // Receive and handle client commands
    while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0';

        if (strncmp(buffer, "CREATE", 6) == 0) {
            create_quiz(client_fd);  // Trigger quiz creation and return code
        } else if (strncmp(buffer, "JOIN", 4) == 0) {
            char code[CODE_LENGTH + 1];
            sscanf(buffer + 5, "%s", code);  // Assuming JOIN <code>
            join_quiz(client_fd, code);      // Validate code and provide corresponding port
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

    // Assign a new `Quiz` object
    Quiz new_quiz;
    new_quiz.port = PORT + 1 + quiz_count; // Port calculation remains the same
    quiz_count++;

    // Store the quiz using the quiz code as the key
    active_quizzes[quiz_code] = new_quiz;
    quiz_codes_to_ports[quiz_code] = new_quiz.port;

    // Send the quiz code to the client
    char message[100];
    sprintf(message, "Quiz created! Use this code to join: %s\n", quiz_code);
    send(client_fd, message, strlen(message), 0);

    printf("Quiz created with code %s on port %d\n", quiz_code, new_quiz.port);
}

// Function to generate a random quiz code
void generate_quiz_code(char* code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < CODE_LENGTH; ++i) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[CODE_LENGTH] = '\0';  // Null terminate the string
}

// Function to join a quiz with a given code
void join_quiz(int client_fd, const char* code) {
    string quiz_code_str(code);
    if (active_quizzes.find(quiz_code_str) != active_quizzes.end()) {
        // Assign the user to the quiz
        char message[100];
        sprintf(message, "Joining quiz with code: %s\n", code);
        send(client_fd, message, strlen(message), 0);

        // Add the user to the quiz participants
        User new_user;
        new_user.fd = client_fd;
        active_quizzes[quiz_code_str].participants.push_back(new_user);

        // Start the quiz for the user
        send_question_to_participants(quiz_code_str, 0);  // Send the first question
    } else {
        send(client_fd, "Invalid quiz code\n", 18, 0);
    }
}

// Function to send a question to all quiz participants
void send_question_to_participants(const string& quiz_code, int question_index) {
    Quiz& quiz = active_quizzes[quiz_code];

    // Construct question message
    string question = quiz.questions[question_index];
    string answers = "";
    for (const auto& ans : quiz.answers[question_index]) {
        answers += ans + "\n";
    }

    if (!answers.empty()) {
        answers.pop_back(); // Remove the last newline
    }

    // Send the question to all participants
    for (const auto& participant : quiz.participants) {
        if (send(participant.fd, question.c_str(), question.size(), 0) == -1) {
            perror("Send question failed");
        }
        if (send(participant.fd, answers.c_str(), answers.size(), 0) == -1) {
            perror("Send answers failed");
        }
    }
}

// Function to handle answer submission
void handle_answer(int client_fd, int question_index, int answer, const string& quiz_code) {
    auto it = active_quizzes.find(quiz_code);
    if (it == active_quizzes.end()) {
        send(client_fd, "Quiz not found.\n", 16, 0);
        return;
    }

    Quiz& quiz = it->second;

    if (answer == quiz.correct_answers[question_index]) {
        // Correct answer handling
        if (send(client_fd, "Correct answer!\n", 16, 0) == -1) {
            perror("Send correct answer failed");
        }
        for (auto& participant : quiz.participants) {
            if (participant.fd == client_fd) {
                participant.score++;  // Increment user's score
            }
        }
    } else {
        // Incorrect answer handling
        if (send(client_fd, "Incorrect answer.\n", 18, 0) == -1) {
            perror("Send incorrect answer failed");
        }
    }
}

