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
void* handle_client(void* client_socket_ptr) {
    int client_socket = *(int*)client_socket_ptr;
    char client_message[BUFFER_SIZE];

    // lexo gjatesine e emrit te klientit
    int name_length;
    if (read(client_socket, &name_length, sizeof(name_length)) <= 0) {
        perror("Dështoi leximi i gjatësisë së emrit të klientit");
        close(client_socket);
        pthread_exit(NULL);
    }

    // lexo emrin e klientit
    if (read(client_socket, clients[client_count].name, name_length) <= 0) {
        perror("Dështoi leximi i emrit të klientit");
        close(client_socket);
        pthread_exit(NULL);
    } else {
        clients[client_count].name[name_length] = '\0'; 
    }

    printf("%s është në linjë.\n", clients[client_count].name);

    // inkrementoje shumen e klientit
    pthread_mutex_lock(&mutex);
    int current_client_count = client_count;
    client_count++;
    pthread_mutex_unlock(&mutex);

    while (1) {
        memset(client_message, 0, sizeof(client_message));
        if (read(client_socket, client_message, sizeof(client_message)) <= 0) {
            perror("Dështoi lexuimi i mesazhit të klientit.");
            close(client_socket);

            pthread_mutex_lock(&mutex);
            // largo klientin prej vargut
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket == client_socket) {
                    printf("%s është jashtë linje.\n", clients[i].name);
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            pthread_exit(NULL);
        }

        handle_client_message(current_client_count, client_message);
    }
}
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_t client_threads[MAX_CLIENTS];

    // krijo nje socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Dështoi për të krijuar soketat");
        exit(1);
    }

    // vendose adresen e serverit
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    // lidhe socketin
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Dështoi për të lidhur soketat");
        exit(1);
    }

    // shiko per connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Dështoi për të marrë lidhjet");
        exit(1);
    }

    printf("Serveri filloi. Duke pritur për lidhje...\n");

    while (1) {
        // prano connection prej klientit
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Dështoi pranimi i kërkesës së lidhjes nga klienti");
            continue;
        }

        if (client_count == MAX_CLIENTS) {
            printf("Maksimumi i klientëve është arritur. Lidhja refuzohet.\n");
            close(client_socket);
            continue;
        }

        // cakto memorien per thread argumentin e klientit
        int* client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL) {
            perror("Dështoi përcaktimi i memories për pointerin e soketës së klientit.");
            close(client_socket);
            continue;
        }

        // vendose vleren e socketit te klientit
        *client_socket_ptr = client_socket;

        clients[client_count].socket = client_socket;

        // krijo threadin per klientin
        if (pthread_create(&client_threads[client_count], NULL, handle_client, client_socket_ptr) != 0) {
            perror("Dështoi krijimi i thread-it të klientit");
            close(client_socket);
            free(client_socket_ptr);
            continue;
        }
    }

    // mbylle socket serverin
    close(server_socket);

    return 0;
}
