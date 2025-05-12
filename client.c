// client.c
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080
#define NUM_CLIENTS 6 // Number of clients to connect concurrently

void create_client(int id) {
    SOCKET sock;
    struct sockaddr_in server_addr;
    char *message = "Hello from client";
    char buffer[1024];

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("[Client %d] Failed to create socket.\n", id);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("[Client %d] Connection failed.\n", id);
        closesocket(sock);
        WSACleanup();
        return;
    }

    printf("[Client %d] Connected to server.\n", id);

    send(sock, message, (int)strlen(message), 0);
    printf("[Client %d] Sent: %s\n", id, message);

    int recv_size = recv(sock, buffer, sizeof(buffer), 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        printf("[Client %d] Received: %s\n", id, buffer);
    }

    printf("[Client %d] Disconnected from server.\n", id);

    closesocket(sock);
    WSACleanup();
}

DWORD WINAPI client_thread(LPVOID lpParam) {
    int id = *(int*)lpParam;
    create_client(id);
    return 0;
}

int main() {
    HANDLE threads[NUM_CLIENTS];
    int client_ids[NUM_CLIENTS];
    DWORD thread_id;

    printf("Starting %d clients...\n\n", NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        client_ids[i] = i + 1; // Start from Client 1
        threads[i] = CreateThread(NULL, 0, client_thread, &client_ids[i], 0, &thread_id);
        if (threads[i] == NULL) {
            printf("Failed to create thread for client %d\n", i + 1);
        }
    }

    for (int i = 0; i < NUM_CLIENTS; i++) {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    printf("\nAll clients have completed their communication.\n");
    return 0;
}
