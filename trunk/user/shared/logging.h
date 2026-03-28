/*
 * Structured Logging System
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stddef.h>

#ifndef __func__
#define __func__ __FUNCTION__
#endif

/*
 * Log levels
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
} log_level_t;

/*
 * Log categories
 */
typedef enum {
    LOG_CAT_SYSTEM   = 0,   /* System initialization, shutdown */
    LOG_CAT_NETWORK  = 1,   /* Network configuration, WAN/LAN */
    LOG_CAT_FIREWALL = 2,   /* Firewall rules, NAT */
    LOG_CAT_DHCP     = 3,   /* DHCP server/client */
    LOG_CAT_WIFI     = 4,   /* WiFi configuration */
    LOG_CAT_VPN      = 5,   /* VPN client/server */
    LOG_CAT_USB      = 6,   /* USB devices */
    LOG_CAT_STORAGE  = 7,   /* Storage, file systems */
    LOG_CAT_SERVICE  = 8,   /* Service management */
    LOG_CAT_AUTH     = 9,   /* Authentication */
    LOG_CAT_HTTPD    = 10,  /* Web interface */
    LOG_CAT_MAX
} log_category_t;

/*
 * Log configuration
 */
struct log_config {
    log_level_t min_level;          /* Minimum level to log */
    int log_to_console;             /* Log to console */
    int log_to_syslog;              /* Log to syslog */
    int log_to_file;                /* Log to file */
    char log_file[256];             /* Log file path */
    int max_log_size;               /* Max log file size in KB */
    int category_mask;              /* Bitmask of enabled categories */
};

/*
 * Initialize logging system
 */
int log_init(void);

/*
 * Close logging system
 */
void log_close(void);

/*
 * Set log configuration
 */
int log_set_config(const struct log_config *config);

/*
 * Get current log configuration
 */
int log_get_config(struct log_config *config);

/*
 * Core logging function
 * @param level: Log level
 * @param cat: Log category
 * @param func: Function name (__func__)
 * @param line: Line number (__LINE__)
 * @param fmt: Printf-style format string
 */
void log_msg(log_level_t level, log_category_t cat,
             const char *func, int line,
             const char *fmt, ...);

/*
 * Convenience macros
 */
#define LOG_DBG(cat, fmt, ...) \
    log_msg(LOG_LEVEL_DEBUG, cat, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INF(cat, fmt, ...) \
    log_msg(LOG_LEVEL_INFO, cat, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WRN(cat, fmt, ...) \
    log_msg(LOG_LEVEL_WARN, cat, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR_MSG(cat, fmt, ...) \
    log_msg(LOG_LEVEL_ERROR, cat, __func__, __LINE__, fmt, ##__VA_ARGS__)

/*
 * Category name for log output
 */
const char *log_category_name(log_category_t cat);

/*
 * Level name for log output
 */
const char *log_level_name(log_level_t level);

/*
 * Enable/disable category
 */
void log_enable_category(log_category_t cat, int enable);

/*
 * Set minimum log level
 */
void log_set_min_level(log_level_t level);

/*
 * Log hexdump (for debugging)
 */
void log_hexdump(log_level_t level, log_category_t cat,
                 const char *func, int line,
                 const void *data, size_t len,
                 const char *title);

#define LOG_HEXDUMP(cat, data, len, title) \
    log_hexdump(LOG_LEVEL_DEBUG, cat, __func__, __LINE__, data, len, title)

#endif /* _LOGGING_H_ */
