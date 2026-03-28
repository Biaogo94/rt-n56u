/*
 * NVRAM Batch Operations API
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _NVRAM_BATCH_H_
#define _NVRAM_BATCH_H_

#include <stddef.h>

/* NVRAM batch item types */
#define NVRAM_TYPE_INT     0
#define NVRAM_TYPE_STRING  1

/* NVRAM batch operation item */
struct nvram_batch_item {
    const char *key;
    int type;                   /* NVRAM_TYPE_INT or NVRAM_TYPE_STRING */
    union {
        int *int_val;
        char **str_val;
        const char *set_val;    /* For set operations */
    } value;
    union {
        struct {
            int def_val;        /* Default value for int */
            int min_val;        /* Min value (0 = no check) */
            int max_val;        /* Max value (0 = no check) */
        } i;
        struct {
            char *buf;          /* Buffer for string result */
            size_t buf_size;    /* Buffer size */
            const char *def_val;/* Default string */
        } s;
    } param;
};

/*
 * nvram_get_batch - Read multiple NVRAM values in one operation
 * @items: Array of nvram_batch_item structures
 * @count: Number of items in array
 * @return: 0 on success, -1 on error
 *
 * This is more efficient than multiple nvram_get() calls
 * as it reuses the file descriptor.
 */
int nvram_get_batch(struct nvram_batch_item *items, int count);

/*
 * nvram_set_batch - Write multiple NVRAM values in one operation
 * @items: Array of nvram_batch_item structures
 * @count: Number of items in array
 * @return: 0 on success, -1 on error
 */
int nvram_set_batch(struct nvram_batch_item *items, int count);

/*
 * nvram_set_batch_commit - Write multiple values and commit
 * @items: Array of nvram_batch_item structures
 * @count: Number of items in array
 * @return: 0 on success, -1 on error
 */
int nvram_set_batch_commit(struct nvram_batch_item *items, int count);

/* Helper macros for batch operations */

/* Define an int item for batch get */
#define NVRAM_GET_INT(key, ptr, defval, minval, maxval) \
    { .key = key, .type = NVRAM_TYPE_INT, .value.int_val = ptr, \
      .param.i = { .def_val = defval, .min_val = minval, .max_val = maxval } }

/* Define a string item for batch get */
#define NVRAM_GET_STR(key, buf, bufsize, defval) \
    { .key = key, .type = NVRAM_TYPE_STRING, .value.str_val = &(buf), \
      .param.s = { .buf = buf, .buf_size = bufsize, .def_val = defval } }

/* Define an int item for batch set */
#define NVRAM_SET_INT(key, val) \
    { .key = key, .type = NVRAM_TYPE_INT, .value.set_val = (const char *)(long)val }

/* Define a string item for batch set */
#define NVRAM_SET_STR(key, val) \
    { .key = key, .type = NVRAM_TYPE_STRING, .value.set_val = val }

#endif /* _NVRAM_BATCH_H_ */
