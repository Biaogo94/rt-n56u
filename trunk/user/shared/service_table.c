/*
 * Service Management Framework Implementation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service_table.h"
#include "nvram_linux.h"
#include "shutils.h"
#include "pids.h"

/*
 * Define all services here
 * This table should be customized based on build configuration
 */

/* Forward declarations for service-specific functions */
#if defined(APP_SSHD)
extern int is_sshd_run(void);
#endif

#if defined(APP_TTYD)
extern int is_ttyd_run(void);
#endif

#if defined(APP_DNSMASQ)
extern int is_dnsmasq_run(void);
#endif

/* Service table */
static const struct service_desc service_table[] = {
    /* Core services */
#if defined(APP_SYSLOGD)
    {
        .name = "syslogd",
        .script = "/sbin/syslogd",
        .pid_name = "syslogd",
        .nvram_enable = NULL,  /* Always on */
        .flags = SVC_FLAG_NONE,
    },
#endif

#if defined(APP_KLOGD)
    {
        .name = "klogd",
        .script = "/sbin/klogd",
        .pid_name = "klogd",
        .nvram_enable = NULL,
        .flags = SVC_FLAG_NONE,
    },
#endif

#if defined(APP_DNSMASQ)
    {
        .name = "dnsmasq",
        .script = "/usr/sbin/dnsmasq",
        .pid_name = "dnsmasq",
        .nvram_enable = NULL,
        .flags = SVC_FLAG_NONE,
    },
#endif

#if defined(APP_DROPBEAR) || defined(USE_SSH)
    {
        .name = "sshd",
        .script = "/usr/bin/sshd.sh",
        .pid_name = "dropbear",
        .nvram_enable = "sshd_enable",
        .flags = SVC_FLAG_SINGLETON | SVC_FLAG_RESTART_FW,
    },
#endif

#if defined(APP_TTYD)
    {
        .name = "ttyd",
        .script = "/usr/bin/ttyd.sh",
        .pid_name = "ttyd",
        .nvram_enable = "ttyd_enable",
        .flags = SVC_FLAG_SINGLETON,
    },
#endif

#if defined(APP_HTTPD)
    {
        .name = "httpd",
        .pid_name = "httpd",
        .nvram_enable = NULL,
        .flags = SVC_FLAG_SINGLETON,
    },
#endif

#if defined(APP_CROND)
    {
        .name = "crond",
        .script = "/usr/sbin/crond",
        .pid_name = "crond",
        .nvram_enable = "crond_enable",
        .flags = SVC_FLAG_SINGLETON,
    },
#endif

#if defined(APP_NTPCLIENT)
    {
        .name = "ntpclient",
        .script = "/usr/sbin/ntpclient.sh",
        .pid_name = "ntpclient",
        .nvram_enable = "ntp_client_enable",
        .flags = SVC_FLAG_LATE_START,
    },
#endif

#if defined(APP_DDNS)
    {
        .name = "inadyn",
        .script = "/usr/sbin/inadyn.sh",
        .pid_name = "inadyn",
        .nvram_enable = "ddns_enable_x",
        .flags = SVC_FLAG_LATE_START,
    },
#endif

#if defined(APP_ARIA2)
    {
        .name = "aria2",
        .script = "/usr/bin/aria2.sh",
        .pid_name = "aria2c",
        .nvram_enable = "aria2_enable",
        .flags = SVC_FLAG_LATE_START,
    },
#endif

#if defined(APP_FRP)
    {
        .name = "frpc",
        .script = "/usr/bin/frpc.sh",
        .pid_name = "frpc",
        .nvram_enable = "frpc_enable",
        .flags = SVC_FLAG_LATE_START,
    },
    {
        .name = "frps",
        .script = "/usr/bin/frps.sh",
        .pid_name = "frps",
        .nvram_enable = "frps_enable",
        .flags = SVC_FLAG_LATE_START,
    },
#endif

#if defined(APP_VLMCSD)
    {
        .name = "vlmcsd",
        .script = "/usr/bin/vlmcsd.sh",
        .pid_name = "vlmcsd",
        .nvram_enable = "vlmcsd_enable",
        .flags = SVC_FLAG_SINGLETON,
    },
#endif

#if defined(APP_SHADOWSOCKS)
    {
        .name = "ss-redir",
        .script = "/usr/bin/ss-redir.sh",
        .pid_name = "ss-redir",
        .nvram_enable = "ss_enable",
        .flags = SVC_FLAG_RESTART_FW,
    },
#endif

    /* Sentinel */
    { .name = NULL }
};

/* Runtime state */
static struct service_entry *service_entries = NULL;
static int service_count_cache = 0;

int
service_init(void)
{
    int count = 0;
    const struct service_desc *desc;

    /* Count services */
    for (desc = service_table; desc->name; desc++)
        count++;

    service_count_cache = count;

    /* Allocate entries */
    service_entries = calloc(count, sizeof(struct service_entry));
    if (!service_entries)
        return -1;

    /* Initialize entries */
    for (int i = 0; i < count; i++) {
        service_entries[i].desc = &service_table[i];
        service_entries[i].is_running = 0;
        service_entries[i].last_start_result = 0;
    }

    return 0;
}

const struct service_desc *
service_find(const char *name)
{
    const struct service_desc *desc;

    if (!name)
        return NULL;

    for (desc = service_table; desc->name; desc++) {
        if (strcmp(desc->name, name) == 0)
            return desc;
    }

    return NULL;
}

int
service_is_running(const struct service_desc *svc)
{
    if (!svc)
        return 0;

    /* Use custom check function if available */
    if (svc->check_running)
        return svc->check_running();

    /* Use pid name if available */
    if (svc->pid_name)
        return pids((char *)svc->pid_name);

    return 0;
}

int
service_is_enabled(const struct service_desc *svc)
{
    int val;

    if (!svc)
        return 0;

    /* No NVRAM key means always enabled */
    if (!svc->nvram_enable)
        return 1;

    val = nvram_get_int(svc->nvram_enable);

    /* Compare with enable value (default 1) */
    if (svc->enable_value)
        return (val == svc->enable_value);

    return (val == 1);
}

int
service_start(const struct service_desc *svc)
{
    int ret = 0;

    if (!svc)
        return -1;

    /* Check if already running for singletons */
    if ((svc->flags & SVC_FLAG_SINGLETON) && service_is_running(svc))
        return 0;

    /* Call pre-start hook */
    if (svc->pre_start)
        svc->pre_start();

    /* Start service */
    if (svc->start_func) {
        ret = svc->start_func();
    } else if (svc->script) {
        ret = eval((char *)svc->script, "start");
    }

    return ret;
}

int
service_stop(const struct service_desc *svc)
{
    int ret = 0;

    if (!svc)
        return -1;

    /* Stop service */
    if (svc->stop_func) {
        ret = svc->stop_func();
    } else if (svc->script) {
        ret = eval((char *)svc->script, "stop");
    } else if (svc->pid_name) {
        /* Kill by process name */
        doSystem("killall -q %s", svc->pid_name);
        ret = 0;
    }

    /* Call post-stop hook */
    if (svc->post_stop)
        svc->post_stop();

    return ret;
}

int
service_restart(const struct service_desc *svc)
{
    int ret;

    if (!svc)
        return -1;

    ret = service_stop(svc);
    if (ret != 0)
        return ret;

    sleep(1);

    return service_start(svc);
}

int
service_start_all(void)
{
    const struct service_desc *desc;
    int count = 0;

    for (desc = service_table; desc->name; desc++) {
        if (service_is_enabled(desc)) {
            if (service_start(desc) == 0)
                count++;
        }
    }

    return count;
}

int
service_stop_all(void)
{
    const struct service_desc *desc;
    int count = 0;

    for (desc = service_table; desc->name; desc++) {
        if (service_is_running(desc)) {
            if (service_stop(desc) == 0)
                count++;
        }
    }

    return count;
}

int
service_restart_by_flags(int flags)
{
    const struct service_desc *desc;
    int count = 0;

    for (desc = service_table; desc->name; desc++) {
        if ((desc->flags & flags) && service_is_running(desc)) {
            if (service_restart(desc) == 0)
                count++;
        }
    }

    return count;
}

void
service_update_states(void)
{
    int i;

    if (!service_entries)
        return;

    for (i = 0; i < service_count_cache; i++) {
        service_entries[i].is_running = service_is_running(service_entries[i].desc);
    }
}

const struct service_entry *
service_get_entry(int index)
{
    if (!service_entries || index < 0 || index >= service_count_cache)
        return NULL;

    return &service_entries[index];
}

int
service_count(void)
{
    return service_count_cache;
}

int
service_start_if_enabled(const char *name)
{
    const struct service_desc *svc = service_find(name);
    if (!svc)
        return -1;

    if (service_is_enabled(svc))
        return service_start(svc);

    return 0;
}

int
service_stop_and_disable(const char *name)
{
    const struct service_desc *svc = service_find(name);
    if (!svc)
        return -1;

    service_stop(svc);

    if (svc->nvram_enable)
        nvram_set(svc->nvram_enable, "0");

    return 0;
}

int
service_toggle(const char *name, int enable)
{
    const struct service_desc *svc = service_find(name);
    if (!svc)
        return -1;

    if (enable) {
        return service_start(svc);
    } else {
        return service_stop(svc);
    }
}
