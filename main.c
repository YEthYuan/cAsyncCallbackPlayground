#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define PORT 8888

typedef struct {
    char *str;
    int client_socket;
    void (*callback)(int client_socket, int result);
} Task;

// Asynchronous task function
void* async_task(void* arg) {
    Task* task = (Task*)arg;
    printf("Async task started with data: %s\n", task->str);

    // Simulate some processing
    sleep(2);

    // Processing finished, call the callback with a result
    task->callback(task->client_socket, (int) strlen(task->str)); // example result: data * 2

    // Clean up and finish
    free(task->str);
    free(task);
    return NULL;
}

// Callback function
void on_task_finished(int client_socket, int result) {
    printf("Callback called with result: %d\n", result);
    char buffer[1024];
    sprintf(buffer, "Message received, length: %d", result);
    send(client_socket, buffer, strlen(buffer), 0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1024];

    fd_set read_fds;
    int max_fd;
    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_sockets[i] = 0;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        max_fd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0)
                FD_SET(client_sockets[i], &read_fds);
            if (client_sockets[i] > max_fd)
                max_fd = client_sockets[i];
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(client_sockets[i], &read_fds)) {
                memset(buffer, 0, sizeof(buffer));
                if (recv(client_sockets[i], buffer, sizeof(buffer), 0) <= 0) {
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                } else {
                    printf("Received from client %d: %s\n", i, buffer);
                    Task *task = (Task*) malloc(sizeof(Task));

                    task->str = (char*) malloc(sizeof(buffer));
                    strcpy(task->str, buffer);
                    task->client_socket = client_sockets[i];
                    task->callback = on_task_finished;

                    pthread_t thread;
                    if (pthread_create(&thread, NULL, async_task, task) != 0) {
                        perror("Failed to create thread");
                        free(task);
                        return 1;
                    }

                    printf("Finished processing client %d's request.\n", i);
                }
            }
        }
    }

    return 0;
}
