#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 1235
#define MAX_CLIENTS 4

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_sockets[MAX_CLIENTS];
int client_count = 0;
int current_turn = 0;  // Indica il giocatore che deve fare il prossimo lancio

//by Luca: error function, execute if error occurs (must pass an "msg" parameter)
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//by Luca: function to accept anc connect clients
void add_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS)
    {
        client_count++;
        client_sockets[client_count] = client_socket;
        //printf("numero giocatori: %d", client_count);
    }
    pthread_mutex_unlock(&clients_mutex);
}

//by Luca: function to remove a client and clean up once disconnected
void remove_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (client_sockets[i] == client_socket)
        {
            client_count--;
            client_sockets[i] = client_sockets[client_count];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//by Luca: function to broadcast A MESSAGE FROM A PLAYER to all clients(includes sender socket)
//NOTE: DELETE IF NEVER USED
void broadcastPlayer(const char *message, int sender_socket)
{
    char buffer[512]; // Buffer to hold the message with sender info
    snprintf(buffer, sizeof(buffer), "Giocatore %d: %s", sender_socket, message);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (client_sockets[i] != sender_socket)
        {
            write(client_sockets[i], buffer, strlen(buffer));
            write(client_sockets[i], "\n", 1);//write new line
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//by Luca: function to broadcast a message notification from server --> all clients
void broadcastNotif(const char *message)
{
    char buffer[512]; // Buffer to hold the message
    snprintf(buffer, sizeof(buffer), "Server notif: %s", message);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i <= client_count; i++)
    {
            write(client_sockets[i], buffer, strlen(buffer));
            write(client_sockets[i], "\n", 1);//write new line
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Funzione per inviare il messaggio di avvio
void start_game() {
    char start_message[] = "Il gioco è iniziato! Prepara i dadi!\n";
    broadcastNotif(start_message);

    // Invio della griglia iniziale (un esempio semplice)
    char grid[] = "Griglia di gioco: [ ] [ ] [ ] [ ] [ ]\n";
    broadcastNotif(grid);
}

//by Luca: function to constantly reading messages sent by a client and pass them to the server
// includes *funzione da definire* --> recognize and react to commands
void readingloop(int client_socket)
{
    char buffer[256];
    int n;

    while (1)
    {
        bzero(buffer, 256);
        n = read(client_socket, buffer, 255);
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        else if (n == 0)
        {
            snprintf(buffer, sizeof(buffer), "Giocatore %d disconnesso.", client_socket);
            printf("Client %d disconnected\n", client_socket);
            broadcastNotif(buffer);
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        printf("messaggio ricevuto da %d: %s\n",client_socket, buffer);
        // Broadcast player message to all clients
        ///broadcastPlayer(buffer, client_socket);
        // React to a command
        //commands(buffer, client_socket);

    }
    close(client_socket);
    remove_client(client_socket);
}

//by Luca: function to handle a single client,
// includes add_client() --> add the client to list and count++
// includes readingloop() --> loop function to read messages it sends
void *clientThread(void *arg)
{
    char buffer[256]; //only to hold message for new player
    int client_socket = *(int *)arg;
    free(arg);
    if (client_count > MAX_CLIENTS) {
        printf("Connection refused: Server full\n");
        snprintf(buffer, sizeof(buffer), "Connessione rifiutata: lobby piena");
        send(client_socket, buffer, strlen(buffer), 0); // Send to the refused client
        close(client_socket); // Close the socket for the refused client
        return NULL;
    }
    printf("New client connected: %d \n", client_socket);
    add_client(client_socket);

    //by Gabri: Invia un messaggio di benvenuto al client
    char welcome_message[] = "---- BENVENUTO! Sei connesso al server! Aspetta che il gioco inizi... ----\n";
    send(client_socket, welcome_message, strlen(welcome_message), 0);

    //notify all players that a new player had joined
    snprintf(buffer, sizeof(buffer), "Giocatore %d connesso.", client_socket);
    broadcastNotif(buffer);

    //start reading client input
    readingloop(client_socket);

    if (client_count == MAX_CLIENTS)
        start_game();

    return NULL;
}

//by Luca: accepting loop that creates a thread for each client connects
void acceptloop(int server_socket)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    pthread_t clTh;

    while (client_count < MAX_CLIENTS)
    {
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&cli_addr, &clilen);
        if (*client_socket < 0)
        {
            free(client_socket);
            error("ERROR on accept");
        }
        else
        {
            pthread_create(&clTh, NULL, clientThread, client_socket);
            pthread_detach(clTh); //makes sure that once terminated, threads clean up its resources automatically
        }
    }
}
/*
// Funzione per inviare la griglia a tutti i client
void broadcast_grid(char *grid) {
    broadcastNotif(grid);
}

// Funzione per lanciare i dadi
int roll_dice() {
    return rand() % 6 + 1;  // Ritorna un numero tra 1 e 6
}
*/

int main() {
    //server related code
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
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Associa l'indirizzo al socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Ascolta per connessioni in entrata
    listen(server_socket, MAX_CLIENTS);
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    //initialize acceptloop
    acceptloop(server_socket);

    // Quando tutti i client sono connessi, inizia il gioco
    if (client_count == MAX_CLIENTS) {
        start_game();  // Inizia il gioco
    }
/*
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
    */
    close(server_socket);
    return 0;
}
