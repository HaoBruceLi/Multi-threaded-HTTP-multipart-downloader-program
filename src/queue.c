#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


/*
 * Queue - the abstract type of a concurrent queue.
 * You must provide an implementation of this type
 * but it is hidden from the outside.
 */
typedef struct QueueStruct {

    int start;            //queue start
    int end;              //queue end
    size_t size;          //queue size

    sem_t queue_add;       //Semaphore for adding to the queue
    sem_t queue_delete;    //Semaphore for deleting from the queue
    pthread_mutex_t mutex_lock;  //set mutex lock

    void **data;
} Queue;


/**
 * Allocate a concurrent queue of a specific size
 * @param size - The size of memory to allocate to the queue
 * @return queue - Pointer to the allocated queue
 */
Queue *queue_alloc(int size) {


    Queue *queue = (Queue *)calloc(1, sizeof(Queue)); // Allocate a concurrent queue

    queue->start = 0;                                 // init strut parameter
    queue->end = 0;
    queue->size = (size_t)size;
    queue->data = (void **)calloc((size_t) size, sizeof(void *));

    pthread_mutex_init(&queue->mutex_lock, NULL);     // init semaphores, mutex
    sem_init(&queue->queue_add, 0, size);
    sem_init(&queue->queue_delete, 0, 0);

    return queue;

}


/**
 * Free a concurrent queue and associated memory
 *
 * Don't call this function while the queue is still in use.
 * (Note, this is a pre-condition to the function and does not need
 * to be checked)
 *
 * @param queue - Pointer to the queue to free
 */
void queue_free(Queue *queue) {

    free(queue->data);
    free(queue);

}


/**
 * Place an item into the concurrent queue.
 * If no space available then queue will block
 * until a space is available when it will
 * put the item into the queue and immediatly return
 *
 * @param queue - Pointer to the queue to add an item to
 * @param item - An item to add to queue. Uses void* to hold an arbitrary
 *               type. User's responsibility to manage memory and ensure
 *               it is correctly typed.
 */
void queue_put(Queue *queue, void *item) {

    sem_wait(&(queue->queue_add));             // if full, decrementing sem, block until sem_post called
    pthread_mutex_lock(&(queue->mutex_lock));  //lock when try to put item
    queue->data[queue->end] = item;
    //printf("%d\n", queue->end);
    if (queue->end + 1 <= queue->size-1){
        queue->end++;
    }
    else{
        queue->end = 0;
    }
    //printf("%d %ld\n", queue->end, queue->size);

    pthread_mutex_unlock(&(queue->mutex_lock)); //increment and unlock after putting item
    sem_post(&(queue->queue_delete));


}


/**
 * Get an item from the concurrent queue
 *
 * If there is no item available then queue_get
 * will block until an item becomes avaible when
 * it will immediately return that item.
 *
 * @param queue - Pointer to queue to get item from
 * @return item - item retrieved from queue. void* type since it can be
 *                arbitrary
 */
void *queue_get(Queue *queue) {

    sem_wait(&queue->queue_delete);   // if empty, block until sem_post called

    pthread_mutex_lock(&queue->mutex_lock);    // lock when thread try to consume data in queue, againist deadlock
    void *item = queue->data[queue->start];

   if (queue->start + 1 <= queue->size-1){
        queue->start++;
    }
    else{
        queue->start = 0;
    }
    pthread_mutex_unlock(&queue->mutex_lock);  // increment and unlock after popping item
    sem_post(&queue->queue_add);
    return item;


}

