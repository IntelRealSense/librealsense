// License: Apache 2.0. See LICENSE file in root directory.
//
#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <inttypes.h>



    typedef struct buf_element_s buf_element_t;


    struct buf_element_s
    {
        buf_element_t        *next;

        unsigned char        *mem;
        unsigned char        *content;   /* start of raw content in mem (without header etc) */

        int64_t               pts ;
        int32_t               size ;     /* size of _content_                                     */
        int duration;
        int32_t               max_size;  /* size of pre-allocated memory pointed to by "mem"      */
        uint32_t              type;
        void (*free_buffer) (buf_element_t *buf);
        void                 *source;   /* pointer to source of this buffer for */
        int					id;
    };


    typedef struct fifo_s fifo_t;
    typedef struct fifo_s fifo_buffer_t;
    struct fifo_s
    {
        buf_element_t  *first, *last;

        int             fifo_size;
        uint32_t        fifo_data_size;
        void            *fifo_empty_cb_data;

        pthread_mutex_t mutex;
        pthread_cond_t  not_empty;

        /*
         * functions to access this fifo:
         */

        void (*put) (fifo_t *fifo, buf_element_t *buf);

        buf_element_t *(*get) (fifo_t *fifo);

        buf_element_t *(*try_get) (fifo_t *fifo);

        void (*clear) (fifo_t *fifo) ;

        int (*size) (fifo_t *fifo);

        int (*num_free) (fifo_t *fifo);

        uint32_t (*data_size) (fifo_t *fifo);

        void (*destroy) (fifo_t *fifo);

        buf_element_t *(*buffer_alloc) (fifo_t *self);

        /* the same as buffer_alloc but may fail if none is available */
        buf_element_t *(*buffer_try_alloc) (fifo_t *self);

        /* the same as put but insert at the head of the fifo */
        void (*insert) (fifo_t *fifo, buf_element_t *buf);

        /*
         * private variables for buffer pool management
         */
        buf_element_t   *buffer_pool_top;    /* a stack actually */
        pthread_mutex_t  buffer_pool_mutex;
        pthread_cond_t   buffer_pool_cond_not_empty;
        int              buffer_pool_num_free;
        int              buffer_pool_capacity;
        int              buffer_pool_buf_size;
        void            *buffer_pool_base; /*used to free mem chunk */
    };

    /*
     * allocate and initialize new (empty) fifo buffer,
     * init buffer pool for it:
     * allocate num_buffers of buf_size bytes each
     */

    fifo_t *fifo_new (int num_buffers, uint32_t buf_size);

#ifdef __cplusplus
}
#endif

#endif
