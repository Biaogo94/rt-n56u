/*
 * Structured Logging Implementation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <syslog.h>
#include "logging.h"

/* Default configuration */
static struct log_config log_cfg = {
    LOG_LEVEL_INFO,
    1,
    1,
    0,
    "/tmp/router.log",
    64,
    0xFFFFFFFF
};

static int log_initialized = 0;

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static const char *category_names[] = {
    "SYSTEM", "NETWORK", "FIREWALL", "DHCP", "WIFI",
    "VPN", "USB", "STORAGE", "SERVICE", "AUTH", "HTTPD"
};

int
log_init(void)
{
    if (log_initialized)
        return 0;

    if (log_cfg.log_to_syslog) {
        openlog("router", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }

    log_initialized = 1;
    return 0;
}

void
log_close(void)
{
    if (!log_initialized)
        return;

    if (log_cfg.log_to_syslog) {
        closelog();
    }

    log_initialized = 0;
}

int
log_set_config(const struct log_config *config)
{
    if (!config)
        return -1;

    memcpy(&log_cfg, config, sizeof(log_cfg));
    return 0;
}

int
log_get_config(struct log_config *config)
{
    if (!config)
        return -1;

    memcpy(config, &log_cfg, sizeof(*config));
    return 0;
}

const char *
log_category_name(log_category_t cat)
{
    if (cat >= LOG_CAT_MAX)
        return "UNKNOWN";
    return category_names[cat];
}

const char *
log_level_name(log_level_t level)
{
    if (level > LOG_LEVEL_ERROR)
        return "UNKNOWN";
    return level_names[level];
}

void
log_enable_category(log_category_t cat, int enable)
{
    if (cat >= LOG_CAT_MAX)
        return;

    if (enable)
        log_cfg.category_mask |= (1 << cat);
    else
        log_cfg.category_mask &= ~(1 << cat);
}

void
log_set_min_level(log_level_t level)
{
    if (level <= LOG_LEVEL_ERROR)
        log_cfg.min_level = level;
}

void
log_msg(log_level_t level, log_category_t cat,
        const char *func, int line,
        const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    char timestamp[32];
    time_t now;
    struct tm *tm_info;
    int syslog_level;

    /* Check if logging is initialized */
    if (!log_initialized)
        log_init();

    /* Check level filter */
    if (level < log_cfg.min_level)
        return;

    /* Check category filter */
    if (cat < LOG_CAT_MAX && !(log_cfg.category_mask & (1 << cat)))
        return;

    /* Format message */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Get timestamp */
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Log to console */
    if (log_cfg.log_to_console) {
        fprintf(stderr, "[%s] %s/%s: %s (%s:%d)\n",
                timestamp, log_level_name(level),
                log_category_name(cat), buf, func, line);
    }

    /* Log to syslog */
    if (log_cfg.log_to_syslog) {
        switch (level) {
            case LOG_LEVEL_DEBUG: syslog_level = LOG_DEBUG; break;
            case LOG_LEVEL_INFO:  syslog_level = LOG_INFO; break;
            case LOG_LEVEL_WARN:  syslog_level = LOG_WARNING; break;
            case LOG_LEVEL_ERROR: syslog_level = LOG_ERR; break;
            default: syslog_level = LOG_INFO; break;
        }
        syslog(syslog_level, "%s/%s: %s",
               log_category_name(cat), func, buf);
    }

    /* Log to file */
    if (log_cfg.log_to_file && log_cfg.log_file[0]) {
        FILE *fp = fopen(log_cfg.log_file, "a");
        if (fp) {
            fprintf(fp, "[%s] %s/%s: %s (%s:%d)\n",
                    timestamp, log_level_name(level),
                    log_category_name(cat), buf, func, line);
            fclose(fp);
        }
    }
}

void
log_hexdump(log_level_t level, log_category_t cat,
            const char *func, int line,
            const void *data, size_t len,
            const char *title)
{
    const unsigned char *p = (const unsigned char *)data;
    char linebuf[128];
    int offset, i;

    if (level < log_cfg.min_level)
        return;

    if (title)
        log_msg(level, cat, func, line, "%s (%lu bytes)", title, (unsigned long)len);

    for (offset = 0; offset < (int)len; offset += 16) {
        int pos = 0;
        pos += snprintf(linebuf + pos, sizeof(linebuf) - pos, "%04x: ", offset);

        for (i = 0; i < 16 && offset + i < (int)len; i++) {
            pos += snprintf(linebuf + pos, sizeof(linebuf) - pos,
                          "%02x ", p[offset + i]);
        }

        for (; i < 16; i++) {
            pos += snprintf(linebuf + pos, sizeof(linebuf) - pos, "   ");
        }

        pos += snprintf(linebuf + pos, sizeof(linebuf) - pos, " ");

        for (i = 0; i < 16 && offset + i < (int)len; i++) {
            unsigned char c = p[offset + i];
            pos += snprintf(linebuf + pos, sizeof(linebuf) - pos,
                          "%c", (c >= 0x20 && c < 0x7f) ? c : '.');
        }

        log_msg(level, cat, func, line, "%s", linebuf);
    }
}
