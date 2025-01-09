#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 4444
#define MAX_CLIENTS 4

int client_sockets[MAX_CLIENTS];
int client_count = 0;
int current_turn = 0;  // Indica il giocatore che deve fare il prossimo lancio

// Funzione per inviare la griglia a tutti i client
void broadcast_grid(char *grid) {
    for (int i = 0; i < client_count; i++) {
        send(client_sockets[i], grid, strlen(grid), 0);
    }
}

// Funzione per lanciare i dadi
int roll_dice() {
    return rand() % 6 + 1;  // Ritorna un numero tra 1 e 6
}

// Funzione per inviare il messaggio di avvio
void start_game() {
    char start_message[] = "Il gioco è iniziato! Prepara i dadi!\n";
    for (int i = 0; i < client_count; i++) {
        send(client_sockets[i], start_message, strlen(start_message), 0);
    }

    // Invio della griglia iniziale (un esempio semplice)
    char grid[] = "Griglia di gioco: [ ] [ ] [ ] [ ] [ ]\n";
    broadcast_grid(grid);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[2048];

    // Crea il socket del server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Associa l'indirizzo al socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Ascolta per connessioni in entrata
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Ciclo per accettare i client
    while (client_count < MAX_CLIENTS) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Aggiungi il client alla lista
        client_sockets[client_count] = client_socket;
        printf("giocatore %d connesso (Socket %d)\n", client_count + 1, client_socket);

        // Incrementa il numero di client connessi
        client_count++;

        // Invia un messaggio di benvenuto al client
        char welcome_message[] = "Sei connesso al server! Aspetta che il gioco inizi...\n";
        send(client_socket, welcome_message, strlen(welcome_message), 0);

        // Quando tutti i client sono connessi, inizia il gioco
        if (client_count == MAX_CLIENTS) {
            start_game();  // Inizia il gioco
        }
    }

    // Gestione dei turni e del lancio dei dadi (da aggiungere qui)
    while (1) {
        // Logica per il turno corrente
        for (int i = 0; i < client_count; i++) {
            if (i == current_turn) {
                // Questo client deve lanciare i dadi
                int dice_result = roll_dice();
                snprintf(buffer, sizeof(buffer), "E' il tuo turno! Hai lanciato il dado: %d\n", dice_result);
                send(client_sockets[i], buffer, strlen(buffer), 0);

                // Aggiorna la griglia (come esempio semplice, aggiungi il numero dei dadi)
                char updated_grid[] = "Griglia aggiornata: [1] [2] [3] [4] [5]\n";
                broadcast_grid(updated_grid);

                // Passa al prossimo giocatore
                current_turn = (current_turn + 1) % client_count;
                break;
            }
        }
        sleep(1);  // Un breve ritardo tra i turni
    }

    close(server_socket);
    return 0;
}
