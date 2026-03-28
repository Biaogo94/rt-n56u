/*
 * Service Management Framework
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _SERVICE_TABLE_H_
#define _SERVICE_TABLE_H_

/*
 * Service flags
 */
#define SVC_FLAG_NONE         0x00
#define SVC_FLAG_RESTART_FW   0x01  /* Requires firewall restart on change */
#define SVC_FLAG_RESTART_NET  0x02  /* Requires network restart on change */
#define SVC_FLAG_SINGLETON    0x04  /* Only one instance allowed */
#define SVC_FLAG_LATE_START   0x08  /* Start after network is up */

/*
 * Service descriptor
 */
struct service_desc {
    const char *name;           /* Service name (for display) */
    const char *script;         /* Path to service script (NULL = internal) */
    const char *pid_name;       /* Process name for pid lookup */
    const char *nvram_enable;   /* NVRAM key for enable/disable (NULL = always) */
    int enable_value;           /* Value that means enabled (default 1) */
    int flags;                  /* Service flags */

    /* Optional custom functions */
    int (*check_running)(void);          /* Custom running check (NULL = use pid_name) */
    int (*start_func)(void);             /* Custom start function */
    int (*stop_func)(void);              /* Custom stop function */
    int (*pre_start)(void);              /* Called before start */
    int (*post_stop)(void);              /* Called after stop */
};

/*
 * Service table entry (runtime state)
 */
struct service_entry {
    const struct service_desc *desc;
    int is_running;             /* Current running state */
    int last_start_result;      /* Last start() result */
};

/*
 * Initialize service framework
 */
int service_init(void);

/*
 * Find service by name
 */
const struct service_desc *service_find(const char *name);

/*
 * Check if service is running
 */
int service_is_running(const struct service_desc *svc);

/*
 * Check if service is enabled
 */
int service_is_enabled(const struct service_desc *svc);

/*
 * Start a service
 */
int service_start(const struct service_desc *svc);

/*
 * Stop a service
 */
int service_stop(const struct service_desc *svc);

/*
 * Restart a service
 */
int service_restart(const struct service_desc *svc);

/*
 * Start all enabled services
 */
int service_start_all(void);

/*
 * Stop all running services
 */
int service_stop_all(void);

/*
 * Restart services affected by a change
 * @param flags: SVC_FLAG_RESTART_FW, SVC_FLAG_RESTART_NET, etc.
 */
int service_restart_by_flags(int flags);

/*
 * Check running state for all services
 */
void service_update_states(void);

/*
 * Get service entry by index (for iteration)
 */
const struct service_entry *service_get_entry(int index);

/*
 * Get total number of services
 */
int service_count(void);

/*
 * Helper macros for defining services
 */

/* Simple script-based service */
#define SERVICE_SIMPLE(name, script, pidname, nvram) \
    { .name = name, .script = script, .pid_name = pidname, .nvram_enable = nvram }

/* Service with custom start/stop */
#define SERVICE_CUSTOM(name, start_fn, stop_fn, check_fn, nvram) \
    { .name = name, .start_func = start_fn, .stop_func = stop_fn, \
      .check_running = check_fn, .nvram_enable = nvram }

/* Service that requires firewall restart */
#define SERVICE_FW(name, script, pidname, nvram) \
    { .name = name, .script = script, .pid_name = pidname, \
      .nvram_enable = nvram, .flags = SVC_FLAG_RESTART_FW }

/* Service that requires network restart */
#define SERVICE_NET(name, script, pidname, nvram) \
    { .name = name, .script = script, .pid_name = pidname, \
      .nvram_enable = nvram, .flags = SVC_FLAG_RESTART_NET }

/*
 * Convenience functions for common patterns
 */

/* Start service if NVRAM key equals value */
int service_start_if_enabled(const char *name);

/* Stop service and disable */
int service_stop_and_disable(const char *name);

/* Toggle service based on NVRAM */
int service_toggle(const char *name, int enable);

#endif /* _SERVICE_TABLE_H_ */
