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
        int *int_val;           /* For integer get operations */
        int set_int_val;        /* For integer set operations */
        const char *set_str_val;/* For string set operations */
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

/*
 * Initialize batch items without relying on C99 designated initializers.
 */
void nvram_batch_item_init_int_get(struct nvram_batch_item *item,
    const char *key, int *ptr, int defval, int minval, int maxval);
void nvram_batch_item_init_str_get(struct nvram_batch_item *item,
    const char *key, char *buf, size_t bufsize, const char *defval);
void nvram_batch_item_init_int_set(struct nvram_batch_item *item,
    const char *key, int val);
void nvram_batch_item_init_str_set(struct nvram_batch_item *item,
    const char *key, const char *val);

#endif /* _NVRAM_BATCH_H_ */
