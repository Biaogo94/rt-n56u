/*
 * Command Batching Implementation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "cmd_batch.h"
#include "shutils.h"

void
cmd_batch_init(struct cmd_batch *batch)
{
    if (!batch)
        return;

    memset(batch, 0, sizeof(struct cmd_batch));
}

int
cmd_batch_add(struct cmd_batch *batch, const char *cmd)
{
    int cmd_len;

    if (!batch || !cmd || !cmd[0])
        return -1;

    cmd_len = strlen(cmd);
    if (cmd_len >= CMD_BATCH_SIZE)
        return -1;

    /* Check if batch is full */
    if (batch->cmd_count >= CMD_BATCH_MAX_CMDS)
        return -1;

    /* Calculate new total length */
    int new_len = batch->total_len;
    if (new_len > 0)
        new_len += 4;  /* " && " separator */
    new_len += cmd_len;

    if (new_len >= CMD_BATCH_SIZE - 1)
        return -1;

    /* Add separator if not first command */
    if (batch->total_len > 0) {
        memcpy(batch->buffer + batch->total_len, " && ", 4);
        batch->total_len += 4;
    }

    /* Add command */
    memcpy(batch->buffer + batch->total_len, cmd, cmd_len);
    batch->total_len += cmd_len;
    batch->buffer[batch->total_len] = '\0';
    batch->cmd_count++;

    return 0;
}

int
cmd_batch_add_independent(struct cmd_batch *batch, const char *cmd)
{
    int cmd_len;

    if (!batch || !cmd || !cmd[0])
        return -1;

    cmd_len = strlen(cmd);
    if (cmd_len >= CMD_BATCH_SIZE)
        return -1;

    /* Check if batch is full */
    if (batch->cmd_count >= CMD_BATCH_MAX_CMDS)
        return -1;

    /* Calculate new total length */
    int new_len = batch->total_len;
    if (new_len > 0)
        new_len += 2;  /* "; " separator */
    new_len += cmd_len;

    if (new_len >= CMD_BATCH_SIZE - 1)
        return -1;

    /* Add separator if not first command */
    if (batch->total_len > 0) {
        memcpy(batch->buffer + batch->total_len, "; ", 2);
        batch->total_len += 2;
    }

    /* Add command */
    memcpy(batch->buffer + batch->total_len, cmd, cmd_len);
    batch->total_len += cmd_len;
    batch->buffer[batch->total_len] = '\0';
    batch->cmd_count++;

    return 0;
}

int
cmd_batch_exec(struct cmd_batch *batch)
{
    int ret;

    if (!batch || batch->total_len == 0)
        return 0;

    ret = system(batch->buffer);

    /* Reset batch after execution */
    cmd_batch_init(batch);

    return ret;
}

void
cmd_batch_exec_async(struct cmd_batch *batch)
{
    pid_t pid;

    if (!batch || batch->total_len == 0)
        return;

    pid = fork();
    if (pid == 0) {
        /* Child process */
        system(batch->buffer);
        _exit(0);
    }

    /* Parent continues, reset batch */
    cmd_batch_init(batch);
}

int
cmd_batch_pending(struct cmd_batch *batch)
{
    if (!batch)
        return 0;

    return batch->cmd_count > 0;
}

void
cmd_batch_clear(struct cmd_batch *batch)
{
    if (batch)
        cmd_batch_init(batch);
}

int
cmd_batch_count(struct cmd_batch *batch)
{
    if (!batch)
        return 0;

    return batch->cmd_count;
}
