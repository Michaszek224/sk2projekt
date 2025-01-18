#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    
    // Powitanie klienta
    const char *welcome_msg = "Witaj! Wybierz: 'host' lub 'gracz'\n";
    send(client_socket, welcome_msg, strlen(welcome_msg), 0);

    // Oczekiwanie na wybór roli
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        close(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';

    // Obsługa roli hosta
    if (strncmp(buffer, "host", 4) == 0) {
        const char *host_msg = "Twój kod hosta to: 1234\n";
        send(client_socket, host_msg, strlen(host_msg), 0);

    // Obsługa roli gracza
    } else if (strncmp(buffer, "gracz", 5) == 0) {
        const char *player_msg = "Połącz na port: 9090\n";
        send(client_socket, player_msg, strlen(player_msg), 0);

    } else {
        const char *invalid_msg = "Niepoprawna rola, rozłączam.\n";
        send(client_socket, invalid_msg, strlen(invalid_msg), 0);
        close(client_socket);
        return;
    }

    // Zamknięcie połączenia
    close(client_socket);
    printf("Klient rozłączony.\n");
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    // Tworzenie socketu serwera
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Nie można utworzyć socketu");
        exit(EXIT_FAILURE);
    }

    // Konfiguracja adresu serwera
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bindowanie adresu do socketu
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Błąd podczas bindowania");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Nasłuchiwanie połączeń
    if (listen(server_socket, 5) < 0) {
        perror("Błąd podczas nasłuchiwania");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d...\n", PORT);

    // Akceptowanie połączeń
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len)) >= 0) {
        printf("Połączono z klientem: %s:%d\n",
               inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));

        handle_client(client_socket);
    }

    if (client_socket < 0) {
        perror("Błąd podczas akceptacji połączenia");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Zamknięcie socketu serwera
    close(server_socket);
    return 0;
}
