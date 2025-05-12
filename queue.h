// queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include <windows.h>

#define QUEUE_SIZE 100

typedef struct {
    SOCKET items[QUEUE_SIZE];
    int front;
    int rear;
    int count;

    HANDLE mutex;
    HANDLE not_empty;
    HANDLE not_full;
} SocketQueue;

void init_queue(SocketQueue *q);
void enqueue(SocketQueue *q, SOCKET item);
SOCKET dequeue(SocketQueue *q);
void destroy_queue(SocketQueue *q);

#endif
