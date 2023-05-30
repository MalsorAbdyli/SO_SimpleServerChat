#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define BUFFER_SIZE 1024


int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;

    // krijo nje socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Dështoi për të krijuar soketat");
        exit(1);
    }

    // vendose server adresen
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // nderroje me ip adresen tende
    server_addr.sin_port = htons(8888);

    // lidhu me serverin
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Dështoi për t'u lidhur me server.");
        exit(1);
    }

    printf("I lidhur me server.\n");

    // lexo emrin e klientit
    char client_name[20];
    printf("Jepni emrin e juaj të përdoruesit: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = 0; 

    // dergo gjatesine e emrit te klientit tek serveri
    int name_length = strlen(client_name);
    write(client_socket, &name_length, sizeof(name_length));

    // dergo emrin e klientit tek serveri
    write(client_socket, client_name, name_length);

    // krijo nje thread per dergim dhe lexim te mesazheve tek serveri
    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) != 0) {
        perror("Dështoi krijimi i thread-it të pranimit të klientit");
        exit(1);
    }

    // lexo dhe dergo mesazhet tek serveri
    char client_message[BUFFER_SIZE];
    while (1) {
        fgets(client_message, sizeof(client_message), stdin);
        client_message[strcspn(client_message, "\n")] = 0;

        // dergo mesazhin tek serveri
        write(client_socket, client_message, strlen(client_message));

        // shiko nese klienti deshiron te largohet
        if (strcmp(client_message, "/quit") == 0) {
            break;
        }
    }

    // mbylle socketin e klientit
    close(client_socket);

    return 0;
}
