#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[2048];  // Aumenta la dimensione del buffer per la griglia

    // Crea il socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Connetti al server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Ricevi messaggio di avvio del gioco
        bzero(buffer, 2048);  // Pulisci il buffer
        int n = recv(client_socket, buffer, 2047, 0);
        if (n <= 0) {
            printf("Server disconnected\n");
            break;
        }

        printf("Received:\n%s", buffer);

        // Se è il turno del client di lanciare i dadi
        if (strstr(buffer, "Il gioco è iniziato!") != NULL) {
            printf("%s", buffer);
            // Ora il client è pronto per il gioco, inizia a lanciare i dadi quando è il suo turno
            // (la logica di lancio dei dadi è già presente nella parte che ti ho fornito)
        }
    }

    close(client_socket);
    return 0;
}
