// queue.c
#include "queue.h"

void init_queue(SocketQueue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;

    q->mutex = CreateMutex(NULL, FALSE, NULL);
    q->not_empty = CreateEvent(NULL, TRUE, FALSE, NULL);
    q->not_full = CreateEvent(NULL, TRUE, TRUE, NULL);
}

void enqueue(SocketQueue *q, SOCKET item) {
    WaitForSingleObject(q->mutex, INFINITE);

    while (q->count == QUEUE_SIZE) {
        ReleaseMutex(q->mutex);
        WaitForSingleObject(q->not_full, INFINITE);
        WaitForSingleObject(q->mutex, INFINITE);
    }

    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->count++;

    SetEvent(q->not_empty);
    if (q->count == QUEUE_SIZE) {
        ResetEvent(q->not_full);
    }

    ReleaseMutex(q->mutex);
}

SOCKET dequeue(SocketQueue *q) {
    WaitForSingleObject(q->mutex, INFINITE);

    while (q->count == 0) {
        ReleaseMutex(q->mutex);
        WaitForSingleObject(q->not_empty, INFINITE);
        WaitForSingleObject(q->mutex, INFINITE);
    }

    SOCKET item = q->items[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->count--;

    SetEvent(q->not_full);
    if (q->count == 0) {
        ResetEvent(q->not_empty);
    }

    ReleaseMutex(q->mutex);
    return item;
}

void destroy_queue(SocketQueue *q) {
    CloseHandle(q->mutex);
    CloseHandle(q->not_empty);
    CloseHandle(q->not_full);
}
