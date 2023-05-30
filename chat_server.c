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
void handle_client_message(int client_index, char* message) {
    char command[20];
    char recipient[20];
    char client_message[BUFFER_SIZE];

    if (message[0] == '/') {
       sscanf(message, "/%s %[^ ] %[^\n]", command, recipient, client_message);

        if (strcmp(command, "msg") == 0) {
            for (int i = 0; i < client_count; i++) {
                if (strcmp(clients[i].name, recipient) == 0) {
                    send_private_message(clients[client_index].name, clients[i].name, client_message);
                    return;
                }
            }
            
            // nese pranuesi nuk eshte gjetur, lajmero derguesin
            char error_message[BUFFER_SIZE];
            snprintf(error_message, BUFFER_SIZE, "Përdoruesi '%s' nuk është gjetur.", recipient);
            write(clients[client_index].socket, error_message, strlen(error_message) + 1);
        } else if (strcmp(command, "quit") == 0) {
            printf("%s është jashtë linje.\n", clients[client_index].name);
            close(clients[client_index].socket);

            pthread_mutex_lock(&mutex);
            // largo klientin nga vargu
            for (int i = client_index; i < client_count - 1; i++) {
                clients[i] = clients[i + 1];
            }
            client_count--;
            pthread_mutex_unlock(&mutex);
        } else if (strcmp(command, "list") == 0) {
            printf("Klientët në linjë:\n");
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                printf("- %s\n", clients[i].name);
            }
            pthread_mutex_unlock(&mutex);
        }
    } else {
        broadcast_message(clients[client_index].name, message);
    }
}
