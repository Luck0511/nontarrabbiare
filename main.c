#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 1234
#define MAX_CLIENTS 4

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_sockets[MAX_CLIENTS];
int client_count = 0;
int current_turn = 0;  // Indica il giocatore che deve fare il prossimo lancio

// Error function to handle errors (by Luca)
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Function to add a client to the list (by Luca)
// Adds the socket to the client_sockets array and increments client_count
void add_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS){
        client_sockets[client_count] = client_socket;
        client_count++;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Function to remove a client from the list (by Luca)
// Removes the socket from the client_sockets array and decrements client_count
void remove_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++)
    {
        if (client_sockets[i] == client_socket)
        {
            client_sockets[i] = client_sockets[client_count - 1]; // Replace with the last socket
            client_sockets[client_count - 1] = -1;               // Clear the last slot
            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


// Broadcast a notification to all clients (by Luca)
// Sends a message to all connected clients
void broadcastNotif(const char *message)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++){
        write(client_sockets[i], message, strlen(message));
        write(client_sockets[i], "\n", 1);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Start the game by broadcasting a message (by Gabri)
// Informs clients that the game has started and sends the initial game state
void start_game()
{
    char start_message[] = "Il gioco è iniziato! Prepara i dadi!\n";
    broadcastNotif(start_message);

    // Broadcast initial game grid
    char grid[] = "Griglia di gioco: [ ] [ ] [ ] [ ] [ ]\n";
    broadcastNotif(grid);
}

// Function to read messages from a client (by Luca)
// Continuously reads messages from the client and handles disconnection
// includes *funzione da definire* --> recognize and react to commands
void readingloop(int client_socket)
{
    char buffer[256];

    while (1)
    {
        bzero(buffer, 256);
        int n = read(client_socket, buffer, 255);
        if (n <= 0){
            printf("Client %d disconnected\n", client_socket);
            snprintf(buffer, sizeof(buffer), "Giocatore %d disconnesso.", client_socket);
            broadcastNotif(buffer);
            remove_client(client_socket);
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        printf("Messaggio ricevuto da %d: %s\n", client_socket, buffer);
    }

    close(client_socket);
    remove_client(client_socket);
}

// Thread to handle a single client (by Luca/Gabri)
// Manages client connections and triggers the game start if the lobby is full
// includes add_client() --> add the client to list and count++
// includes readingloop() --> loop function to read messages it sends
void *clientThread(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&clients_mutex);
    if (client_count >= MAX_CLIENTS){
        pthread_mutex_unlock(&clients_mutex);
        char full_message[] = "Connessione rifiutata: lobby piena\n";
        send(client_socket, full_message, strlen(full_message), 0);
        close(client_socket);
        return NULL;
    }
    pthread_mutex_unlock(&clients_mutex);

    add_client(client_socket);
    printf("New client connected: %d\n", client_socket);

    char welcome_message[] = "---- BENVENUTO! Sei connesso al server! Aspetta che il gioco inizi... ----\n";
    send(client_socket, welcome_message, strlen(welcome_message), 0);

    char notif_message[256];
    snprintf(notif_message, sizeof(notif_message), "Giocatore %d connesso.", client_socket);
    broadcastNotif(notif_message);

    // Start the game when the maximum number of clients connect
    if (client_count == MAX_CLIENTS){
        start_game();
    }
    //start readingloop() to read client messages
    readingloop(client_socket);

    return NULL;
}

// Function to accept connections and create threads for each client (by Luca/Gabri)
// Accepts client connections until the lobby is full
void acceptloop(int server_socket)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    while (1)
    {
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen);

        if (*client_socket < 0){
            free(client_socket);
            error("ERROR on accept");
        }

        pthread_t clTh;
        pthread_create(&clTh, NULL, clientThread, client_socket);
        pthread_detach(clTh);
    }
}

int main()
{
    int server_socket;
    struct sockaddr_in server_addr;

    // Create a socket (by Gabri)
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the server address structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the port (by Gabri)
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections (by Gabri)
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept client connections
    acceptloop(server_socket);

    // Close the server socket
    close(server_socket);
    return 0;
}
