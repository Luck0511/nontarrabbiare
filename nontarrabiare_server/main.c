#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int clients[MAX_CLIENTS] = {0}; // List of client sockets
int client_count = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void add_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS)
    {
        clients[client_count++] = client_socket;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i] == client_socket)
        {
            clients[i] = clients[--client_count];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast(const char *message, int sender_socket)
{
    char buffer[512]; // Buffer to hold the message with sender info
    snprintf(buffer, sizeof(buffer), "Sent by %d: %s", sender_socket, message);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i] != sender_socket)
        {
            write(clients[i], buffer, strlen(buffer));
            write(clients[i], "\n", 1);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int rolldice(void)
{
    const int rollres = (rand() % 6) + 1;
    return rollres;
}

int commands(char *buffer, int sender_socket)
{
    int rollres;
    if (strcmp(buffer, "roll") == 0){
        broadcast("rolling dices: ", sender_socket);
        rollres = rolldice();
        printf(rollres);
    }
    else if(strcmp(buffer, "hello") == 0) {
        strcpy(buffer, "how are you?");
    }
    else {
        strcpy(buffer, "WTF.. ");
    }

    return strlen(buffer);
}

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
            printf("Client disconnected\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        printf("Received message: %s\n", buffer);
        // Broadcast the response to all clients
        broadcast(buffer, client_socket);
        // React to a command
        commands(buffer, client_socket);


    }

    close(client_socket);
    remove_client(client_socket);
}

void *clientThread(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    printf("Accepted new client\n");
    add_client(client_socket);
    readingloop(client_socket);

    return NULL;
}

void acceptloop(int server_socket)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    pthread_t clTh;

    while (1)
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
            pthread_detach(clTh);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    int sockfd, portno;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    portno = atoi(argv[1]);
    printf("Listening on port %d\n", portno);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    listen(sockfd, 5);
    acceptloop(sockfd);

    close(sockfd);
    return 0;
}
