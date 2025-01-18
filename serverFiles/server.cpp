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

int main() {
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t thread_id;

    // Inicjalizacja gniazda serwera
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

    // Główna pętla serwera
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }
        cout << "New client connected" << endl;

        // Przydzielamy nowemu klientowi wątek
        if (pthread_create(&thread_id, NULL, client_thread, (void *)&new_socket) != 0) {
            perror("Thread creation failed");
        }
        cout << "Thread created" << endl;
    }

    return 0;
}

// Obsługa klienta w osobnym wątku
void *client_thread(void *arg) {
    int client_fd = *((int *)arg);
    char buffer[1024];
    int n;

    // Przyjmujemy dane od klienta (np. wybór akcji: tworzenie quizu / dołączenie do quizu)
    while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0';
        
        if (strncmp(buffer, "CREATE", 6) == 0) {
            // Logika tworzenia nowego quizu (wstawienie pytania, odpowiedzi, rozpoczęcie itp.)
            send(client_fd, "Quiz created\n", 13, 0);
            cout << "Creating quiz" << endl;
        } else if (strncmp(buffer, "JOIN", 4) == 0) {
            // Logika dołączenia do quizu (weryfikacja kodu)
            send(client_fd, "Joined quiz\n", 12, 0);
            cout << "Joining quiz" << endl;
        } else {
            send(client_fd, "Invalid command\n", 16, 0);
        }
    }

    close(client_fd);
    return NULL;
}

