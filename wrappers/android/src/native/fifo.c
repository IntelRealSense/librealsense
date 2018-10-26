// License: Apache 2.0. See LICENSE file in root directory.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "fifo.h"

static void _xfree(void *ptr)
{
    if (NULL != ptr)
    {
        free(ptr);
        ptr = NULL;
    }
}

static void *_xmalloc(size_t size)
{
    void *ptr;

    /* prevent _xmalloc(0) of possibly returning NULL */
    if ( !size )
        size++;

    if ((ptr = calloc(1, size)) == NULL)
    {
        return NULL;
    }

    return ptr;
}

static void *_xmalloc_aligned(size_t alignment, size_t size, void **base)
{

    char *ptr;

    *base = ptr = _xmalloc (size+alignment);

    while ((size_t) ptr % alignment)
        ptr++;

    return ptr;
}


/*
 * put a previously allocated buffer element back into the buffer pool
 */
static void buffer_pool_free (buf_element_t *element)
{

    fifo_t *this = (fifo_t *) element->source;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    element->next = this->buffer_pool_top;
    this->buffer_pool_top = element;

    this->buffer_pool_num_free++;
    if (this->buffer_pool_num_free > this->buffer_pool_capacity)
    {
        /* shouldn't happend, just abort */
        abort();
    }

    pthread_cond_signal (&this->buffer_pool_cond_not_empty);

    pthread_mutex_unlock (&this->buffer_pool_mutex);
}

/*
 * allocate a buffer from buffer pool
 */
static buf_element_t *buffer_pool_alloc (fifo_t *this)
{

    buf_element_t *buf;
    int i;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    /* we always keep one free buffer for emergency situations like
     * decoder flushes that would need a buffer in buffer_pool_try_alloc() */
    while (this->buffer_pool_num_free < 2)
    {
        pthread_cond_wait (&this->buffer_pool_cond_not_empty, &this->buffer_pool_mutex);
    }

    buf = this->buffer_pool_top;
    this->buffer_pool_top = this->buffer_pool_top->next;
    this->buffer_pool_num_free--;

    buf->content = buf->mem;
    buf->size = 0;
    buf->type = 0;
    pthread_mutex_unlock (&this->buffer_pool_mutex);

    return buf;
}


/*
 * allocate a buffer from buffer pool - may fail if none is available
 */
static buf_element_t *buffer_pool_try_alloc (fifo_t *this)
{

    buf_element_t *buf;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    if (this->buffer_pool_top)
    {

        buf = this->buffer_pool_top;
        this->buffer_pool_top = this->buffer_pool_top->next;
        this->buffer_pool_num_free--;

    }
    else
    {

        buf = NULL;

    }

    pthread_mutex_unlock (&this->buffer_pool_mutex);

    if ( buf )
    {
        buf->content = buf->mem;
        buf->size = 0;
    }
    return buf;
}


/*
 * append buffer element to fifo buffer
 */
static void fifo_buffer_put (fifo_t *fifo, buf_element_t *element)
{
    int i;

    pthread_mutex_lock (&fifo->mutex);


    if (fifo->last)
        fifo->last->next = element;
    else
        fifo->first = element;

    fifo->last = element;
    element->next = NULL;
    fifo->fifo_size++;
    fifo->fifo_data_size += element->size;

    pthread_cond_signal (&fifo->not_empty);
    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * append buffer element to fifo buffer
 */
static void dummy_fifo_buffer_put (fifo_t *fifo, buf_element_t *element)
{
    int i;

    pthread_mutex_lock (&fifo->mutex);

    pthread_mutex_unlock (&fifo->mutex);

    element->free_buffer(element);
}

/*
 * insert buffer element to fifo buffer (demuxers MUST NOT call this one)
 */
static void fifo_buffer_insert (fifo_t *fifo, buf_element_t *element)
{

    pthread_mutex_lock (&fifo->mutex);

    element->next = fifo->first;
    fifo->first = element;

    if ( !fifo->last )
        fifo->last = element;

    fifo->fifo_size++;
    fifo->fifo_data_size += element->size;

    pthread_cond_signal (&fifo->not_empty);

    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * insert buffer element to fifo buffer (demuxers MUST NOT call this one)
 */
static void dummy_fifo_buffer_insert (fifo_t *fifo, buf_element_t *element)
{

    element->free_buffer(element);
}

/*
 * get element from fifo buffer
 */
static buf_element_t *fifo_buffer_get (fifo_t *fifo)
{
    int i;
    buf_element_t *buf;

    pthread_mutex_lock (&fifo->mutex);

    while (NULL == fifo->first)
    {
        pthread_cond_wait (&fifo->not_empty, &fifo->mutex);
    }

    buf = fifo->first;

    fifo->first = fifo->first->next;
    if (NULL == fifo->first)
        fifo->last = NULL;

    fifo->fifo_size--;
    fifo->fifo_data_size -= buf->size;

    pthread_mutex_unlock (&fifo->mutex);

    return buf;
}

static buf_element_t *fifo_buffer_try_get(fifo_t *fifo)
{
    int i;
    buf_element_t *buf;

    pthread_mutex_lock (&fifo->mutex);

    if (NULL == fifo->first)
    {
        pthread_mutex_unlock (&fifo->mutex);
        return NULL;
    }

    buf = fifo->first;

    fifo->first = fifo->first->next;
    if (NULL == fifo->first)
        fifo->last = NULL;

    fifo->fifo_size--;
    fifo->fifo_data_size -= buf->size;

    pthread_mutex_unlock (&fifo->mutex);

    return buf;
}


/*
 * clear buffer (put all contained buffer elements back into buffer pool)
 */
static void fifo_buffer_clear (fifo_t *fifo)
{

    buf_element_t *buf, *next, *prev;

    pthread_mutex_lock (&fifo->mutex);

    buf = fifo->first;
    prev = NULL;

    while (buf != NULL)
    {

        next = buf->next;

        {
            /* remove this buffer */
            if (prev)
                prev->next = next;
            else
                fifo->first = next;

            if (!next)
                fifo->last = prev;

            fifo->fifo_size--;
            fifo->fifo_data_size -= buf->size;

            buf->free_buffer(buf);
        }
        buf = next;
    }

    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * Return the number of elements in the fifo buffer
 */
static int fifo_buffer_size (fifo_t *this)
{
    int size;

    pthread_mutex_lock(&this->mutex);
    size = this->fifo_size;
    pthread_mutex_unlock(&this->mutex);

    return size;
}

/*
 * Return the amount of the data in the fifo buffer
 */
static uint32_t fifo_buffer_data_size (fifo_t *this)
{
    uint32_t data_size;

    pthread_mutex_lock(&this->mutex);
    data_size = this->fifo_data_size;
    pthread_mutex_unlock(&this->mutex);

    return data_size;
}

/*
 * Return the number of free elements in the pool
 */
static int fifo_buffer_num_free (fifo_t *this)
{
    int buffer_pool_num_free;

    pthread_mutex_lock(&this->mutex);
    buffer_pool_num_free = this->buffer_pool_num_free;
    pthread_mutex_unlock(&this->mutex);

    return buffer_pool_num_free;
}

/*
 * Destroy the buffer
 */
static void fifo_buffer_dispose (fifo_t *this)
{

    buf_element_t *buf, *next;
    int received = 0;

    this->clear( this );
    buf = this->buffer_pool_top;

    while (buf != NULL)
    {

        next = buf->next;

        free (buf);
        received++;

        buf = next;
    }

    while (received < this->buffer_pool_capacity)
    {

        buf = this->get(this);

        free(buf);
        received++;
    }

    free (this->buffer_pool_base);
    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->not_empty);
    pthread_mutex_destroy(&this->buffer_pool_mutex);
    pthread_cond_destroy(&this->buffer_pool_cond_not_empty);
    free (this);
}


/*
 * allocate and initialize new (empty) fifo buffer
 */
fifo_t *fifo_new (int num_buffers, uint32_t buf_size)
{

    fifo_t *this;
    int            i;
    int            alignment = 1;
    char          *multi_buffer = NULL;

    this = _xmalloc (sizeof (fifo_t));

    this->first               = NULL;
    this->last                = NULL;
    this->fifo_size           = 0;
    this->put                 = fifo_buffer_put;
    this->insert              = fifo_buffer_insert;
    this->get                 = fifo_buffer_get;
    this->try_get              = fifo_buffer_try_get;
    this->clear               = fifo_buffer_clear;
    this->size                = fifo_buffer_size;
    this->num_free            = fifo_buffer_num_free;
    this->data_size           = fifo_buffer_data_size;
    this->destroy             = fifo_buffer_dispose;
    pthread_mutex_init (&this->mutex, NULL);
    pthread_cond_init (&this->not_empty, NULL);

    /*
     * init buffer pool, allocate nNumBuffers of buf_size bytes each
     */
    if (buf_size % alignment != 0)
        buf_size += alignment - (buf_size % alignment);

    multi_buffer = _xmalloc_aligned (alignment, num_buffers * buf_size, &this->buffer_pool_base);

    assert(multi_buffer);
    this->buffer_pool_top = NULL;

    pthread_mutex_init (&this->buffer_pool_mutex, NULL);
    pthread_cond_init (&this->buffer_pool_cond_not_empty, NULL);

    this->buffer_pool_num_free  = 0;
    this->buffer_pool_capacity  = num_buffers;
    this->buffer_pool_buf_size  = buf_size;
    this->buffer_alloc     = buffer_pool_alloc;
    this->buffer_try_alloc = buffer_pool_try_alloc;

    for (i = 0; i<num_buffers; i++)
    {
        buf_element_t *buf;

        buf = _xmalloc (sizeof (buf_element_t));

        buf->id = i;
        buf->mem = (unsigned char*)multi_buffer;
        multi_buffer += buf_size;

        buf->max_size    = buf_size;
        buf->free_buffer = buffer_pool_free;
        buf->source      = this;

        buffer_pool_free (buf);
    }
    return this;
}
