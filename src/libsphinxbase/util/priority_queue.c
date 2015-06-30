
#include <sphinxbase/priority_queue.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>

struct priority_queue_s {
    void **pointers;
    size_t alloc_size;
    size_t size;
    void *max_element;
    int (*compare)(void *a, void *b);
};

priority_queue_t* priority_queue_create(size_t len, int (*compare)(void *a, void *b))
{
    priority_queue_t* queue;

    queue = (priority_queue_t *)ckd_calloc(1, sizeof(*queue));
    queue->alloc_size = len;
    queue->pointers = (void **)ckd_calloc(len, sizeof(*queue->pointers));
    queue->size = 0;
    queue->max_element = NULL;
    queue->compare = compare;

    return queue;
}

void* priority_queue_poll(priority_queue_t *queue)
{
    size_t i;
    void *res;

    if (queue->size == 0) {
        E_WARN("Trying to poll from empty queue\n");
        return NULL;
    }
    if (queue->max_element == NULL) {
        E_ERROR("Trying to poll from queue and max element is undefined\n");
        return NULL;
    }
    res = queue->max_element;
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == queue->max_element) {
            queue->pointers[i] = NULL;
            break;
        }
    }
    queue->max_element = NULL;
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == 0)
            continue;
        if (queue->max_element == NULL) {
            queue->max_element = queue->pointers[i];
        } else {
            if (queue->compare(queue->pointers[i], queue->max_element) > 0)
                queue->max_element = queue->pointers[i];
        }
    }
    queue->size--;
    return res;
}

void priority_queue_add(priority_queue_t *queue, void *element)
{
    size_t i;
    if (queue->size == queue->alloc_size) {
        E_ERROR("Trying to add element into full queue\n");
        return;
    }
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == NULL) {
            queue->pointers[i] = element;
            break;
        }
    }

    if (queue->max_element == NULL || queue->compare(element, queue->max_element) > 0) {
        queue->max_element = element;
    }
    queue->size++;
}

size_t priority_queue_size(priority_queue_t *queue)
{
    return queue->size;
}

void priority_queue_free(priority_queue_t *queue, void (*free_ptr)(void *a))
{
    size_t i;

    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] != NULL) {
            if (free_ptr == NULL) {
                ckd_free(queue->pointers[i]);
            } else {
                free_ptr(queue->pointers[i]);
            }
        }
    }
    ckd_free(queue->pointers);
    ckd_free(queue);
}