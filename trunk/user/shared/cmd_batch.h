/*
 * Command Batching - Reduce fork/exec overhead
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Purpose:
 * - Batch multiple shell commands into single execution
 * - Reduce process creation overhead
 * - Maintain command execution order
 */

#ifndef _CMD_BATCH_H_
#define _CMD_BATCH_H_

#include <stddef.h>

/*
 * Batch buffer configuration
 */
#define CMD_BATCH_SIZE      2048    /* Maximum batch buffer size */
#define CMD_BATCH_MAX_CMDS  32      /* Maximum commands in one batch */

/*
 * Command batch context
 */
struct cmd_batch {
    char buffer[CMD_BATCH_SIZE];
    int cmd_count;
    int total_len;
    int failed;
};

/*
 * Initialize a command batch
 * @param batch: Batch context to initialize
 */
void cmd_batch_init(struct cmd_batch *batch);

/*
 * Add a command to the batch
 * @param batch: Batch context
 * @param cmd: Command to add
 * @return: 0 on success, -1 if batch is full
 *
 * Note: Commands are separated by " && " to ensure
 *       execution stops on first failure
 */
int cmd_batch_add(struct cmd_batch *batch, const char *cmd);

/*
 * Add a command that continues even if previous failed
 * @param batch: Batch context
 * @param cmd: Command to add
 * @return: 0 on success, -1 if batch is full
 *
 * Note: Commands are separated by "; " for independent execution
 */
int cmd_batch_add_independent(struct cmd_batch *batch, const char *cmd);

/*
 * Execute the batch and reset
 * @param batch: Batch context
 * @return: Exit code of the batch execution
 */
int cmd_batch_exec(struct cmd_batch *batch);

/*
 * Execute batch and discard result (fire and forget)
 * @param batch: Batch context
 */
void cmd_batch_exec_async(struct cmd_batch *batch);

/*
 * Check if batch has pending commands
 * @param batch: Batch context
 * @return: 1 if has pending, 0 if empty
 */
int cmd_batch_pending(struct cmd_batch *batch);

/*
 * Clear batch without executing
 * @param batch: Batch context
 */
void cmd_batch_clear(struct cmd_batch *batch);

/*
 * Get number of commands in batch
 * @param batch: Batch context
 * @return: Number of pending commands
 */
int cmd_batch_count(struct cmd_batch *batch);

/*
 * Convenience macros for common patterns
 */

/* Execute multiple commands with batch */
#define BATCH_EXEC(...) do { \
    struct cmd_batch _batch; \
    cmd_batch_init(&_batch); \
    __VA_ARGS__; \
    if (cmd_batch_pending(&_batch)) cmd_batch_exec(&_batch); \
} while(0)

/* Add iptables command to batch */
#define BATCH_IPTABLES(batch, fmt, ...) do { \
    char _cmd[256]; \
    snprintf(_cmd, sizeof(_cmd), "iptables " fmt, ##__VA_ARGS__); \
    cmd_batch_add(batch, _cmd); \
} while(0)

/* Add ip6tables command to batch */
#define BATCH_IP6TABLES(batch, fmt, ...) do { \
    char _cmd[256]; \
    snprintf(_cmd, sizeof(_cmd), "ip6tables " fmt, ##__VA_ARGS__); \
    cmd_batch_add(batch, _cmd); \
} while(0)

#endif /* _CMD_BATCH_H_ */
