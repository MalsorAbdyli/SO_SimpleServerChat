#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char name[20];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char* sender_name, char* message) {
    char broadcast_buffer[BUFFER_SIZE];
    snprintf(broadcast_buffer, BUFFER_SIZE, "%s: %s", sender_name, message);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        write(clients[i].socket, broadcast_buffer, strlen(broadcast_buffer) + 1);
    }
    pthread_mutex_unlock(&mutex);
}

void send_private_message(char* sender_name, char* recipient_name, char* message) {
    char private_buffer[BUFFER_SIZE];
    snprintf(private_buffer, BUFFER_SIZE, "[Mesazh privat nga %s për %s]: %s", sender_name, recipient_name, message);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].name, recipient_name) == 0) {
            write(clients[i].socket, private_buffer, strlen(private_buffer) + 1);
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}
