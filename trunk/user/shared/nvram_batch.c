/*
 * NVRAM Batch Operations Implementation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvram_batch.h"
#include "nvram_linux.h"

static void
nvram_batch_item_reset(struct nvram_batch_item *item)
{
    if (!item)
        return;

    memset(item, 0, sizeof(*item));
}

void
nvram_batch_item_init_int_get(struct nvram_batch_item *item,
    const char *key, int *ptr, int defval, int minval, int maxval)
{
    nvram_batch_item_reset(item);
    if (!item)
        return;

    item->key = key;
    item->type = NVRAM_TYPE_INT;
    item->value.int_val = ptr;
    item->param.i.def_val = defval;
    item->param.i.min_val = minval;
    item->param.i.max_val = maxval;
}

void
nvram_batch_item_init_str_get(struct nvram_batch_item *item,
    const char *key, char *buf, size_t bufsize, const char *defval)
{
    nvram_batch_item_reset(item);
    if (!item)
        return;

    item->key = key;
    item->type = NVRAM_TYPE_STRING;
    item->param.s.buf = buf;
    item->param.s.buf_size = bufsize;
    item->param.s.def_val = defval;
}

void
nvram_batch_item_init_int_set(struct nvram_batch_item *item,
    const char *key, int val)
{
    nvram_batch_item_reset(item);
    if (!item)
        return;

    item->key = key;
    item->type = NVRAM_TYPE_INT;
    item->value.set_int_val = val;
}

void
nvram_batch_item_init_str_set(struct nvram_batch_item *item,
    const char *key, const char *val)
{
    nvram_batch_item_reset(item);
    if (!item)
        return;

    item->key = key;
    item->type = NVRAM_TYPE_STRING;
    item->value.set_str_val = val;
}

int
nvram_get_batch(struct nvram_batch_item *items, int count)
{
    int i;
    char *val;
    struct nvram_batch_item *item;

    if (!items || count <= 0)
        return -1;

    for (i = 0; i < count; i++) {
        item = &items[i];

        if (item->type == NVRAM_TYPE_INT) {
            int v;

            val = nvram_get(item->key);
            if (val && val[0]) {
                v = atoi(val);
                /* Validate range */
                if (item->param.i.min_val != item->param.i.max_val) {
                    if (v < item->param.i.min_val || v > item->param.i.max_val)
                        v = item->param.i.def_val;
                }
                if (item->value.int_val)
                    *item->value.int_val = v;
            } else if (item->value.int_val) {
                *item->value.int_val = item->param.i.def_val;
            }
        } else if (item->type == NVRAM_TYPE_STRING) {
            val = nvram_get(item->key);
            if (item->param.s.buf && item->param.s.buf_size > 0) {
                if (val && val[0]) {
                    snprintf(item->param.s.buf, item->param.s.buf_size, "%s", val);
                } else if (item->param.s.def_val) {
                    snprintf(item->param.s.buf, item->param.s.buf_size, "%s", item->param.s.def_val);
                } else {
                    item->param.s.buf[0] = '\0';
                }
            }
        }
    }

    return 0;
}

int
nvram_set_batch(struct nvram_batch_item *items, int count)
{
    int i;
    char int_buf[16];
    struct nvram_batch_item *item;

    if (!items || count <= 0)
        return -1;

    for (i = 0; i < count; i++) {
        item = &items[i];

        if (item->type == NVRAM_TYPE_INT) {
            snprintf(int_buf, sizeof(int_buf), "%d", item->value.set_int_val);
            nvram_set(item->key, int_buf);
        } else if (item->type == NVRAM_TYPE_STRING) {
            nvram_set(item->key, item->value.set_str_val ? item->value.set_str_val : "");
        }
    }

    return 0;
}

int
nvram_set_batch_commit(struct nvram_batch_item *items, int count)
{
    int ret;

    ret = nvram_set_batch(items, count);
    if (ret == 0)
        nvram_commit();

    return ret;
}
