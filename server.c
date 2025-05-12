// server.c
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "queue.h"

#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080
#define THREAD_POOL_SIZE 4
#define MAX_CONNECTIONS 5

SocketQueue socket_queue;
HANDLE connection_semaphore;

DWORD WINAPI worker_thread(LPVOID lpParam) {
    while (1) {
        SOCKET client_socket = dequeue(&socket_queue);
        char buffer[1024] = {0};

        // Get connection time
        SYSTEMTIME start, end;
        GetSystemTime(&start); // Client connected time

        recv(client_socket, buffer, sizeof(buffer), 0);
        printf("Received from client: %s\n", buffer);

        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 12\r\n\r\n"
            "Hello world";

        send(client_socket, response, (int)strlen(response), 0);

        closesocket(client_socket);

        GetSystemTime(&end); // Client disconnected time
        printf("[Worker Thread] Client served: Start: %02d:%02d:%02d.%03d | End: %02d:%02d:%02d.%03d\n",
               start.wHour, start.wMinute, start.wSecond, start.wMilliseconds,
               end.wHour, end.wMinute, end.wSecond, end.wMilliseconds);

        // Release one spot in the semaphore after serving the client
        ReleaseSemaphore(connection_semaphore, 1, NULL);
    }
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    HANDLE threads[THREAD_POOL_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);
    init_queue(&socket_queue);

    // Create semaphore for connection limit (max 5 concurrent connections)
    connection_semaphore = CreateSemaphore(NULL, MAX_CONNECTIONS, MAX_CONNECTIONS, NULL);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, SOMAXCONN);

    printf("Server listening on port %d (max %d concurrent connections)\n", PORT, MAX_CONNECTIONS);

    // Create worker threads (to process client requests)
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        threads[i] = CreateThread(NULL, 0, worker_thread, NULL, 0, NULL);
    }

    // Accept loop (accept connections and enqueue them for workers)
    while (1) {
        // Wait for a free spot (blocks if 5 clients are being served)
        WaitForSingleObject(connection_semaphore, INFINITE);

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            ReleaseSemaphore(connection_semaphore, 1, NULL); // Restore slot on failure
            continue;
        }

        // Log client connection time
        SYSTEMTIME connect_time;
        GetSystemTime(&connect_time);
        printf("[Server] Client connected at %02d:%02d:%02d.%03d\n",
               connect_time.wHour, connect_time.wMinute, connect_time.wSecond, connect_time.wMilliseconds);

        // Enqueue the client socket to be processed by a worker thread
        enqueue(&socket_queue, client_socket);
    }

    // Cleanup (unreachable here, but good practice)
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    CloseHandle(connection_semaphore);
    closesocket(server_socket);
    destroy_queue(&socket_queue);
    WSACleanup();
    return 0;
}
