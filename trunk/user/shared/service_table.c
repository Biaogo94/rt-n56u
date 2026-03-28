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
#include <unistd.h>
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
    { "syslogd", "/sbin/syslogd", "syslogd", NULL, 0, SVC_FLAG_NONE, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_KLOGD)
    { "klogd", "/sbin/klogd", "klogd", NULL, 0, SVC_FLAG_NONE, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_DNSMASQ)
    { "dnsmasq", "/usr/sbin/dnsmasq", "dnsmasq", NULL, 0, SVC_FLAG_NONE, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_DROPBEAR) || defined(USE_SSH)
    { "sshd", "/usr/bin/sshd.sh", "dropbear", "sshd_enable", 0, SVC_FLAG_SINGLETON | SVC_FLAG_RESTART_FW, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_TTYD)
    { "ttyd", "/usr/bin/ttyd.sh", "ttyd", "ttyd_enable", 0, SVC_FLAG_SINGLETON, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_HTTPD)
    { "httpd", NULL, "httpd", NULL, 0, SVC_FLAG_SINGLETON, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_CROND)
    { "crond", "/usr/sbin/crond", "crond", "crond_enable", 0, SVC_FLAG_SINGLETON, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_NTPCLIENT)
    { "ntpclient", "/usr/sbin/ntpclient.sh", "ntpclient", "ntp_client_enable", 0, SVC_FLAG_LATE_START, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_DDNS)
    { "inadyn", "/usr/sbin/inadyn.sh", "inadyn", "ddns_enable_x", 0, SVC_FLAG_LATE_START, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_ARIA2)
    { "aria2", "/usr/bin/aria2.sh", "aria2c", "aria2_enable", 0, SVC_FLAG_LATE_START, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_FRP)
    { "frpc", "/usr/bin/frpc.sh", "frpc", "frpc_enable", 0, SVC_FLAG_LATE_START, NULL, NULL, NULL, NULL, NULL },
    { "frps", "/usr/bin/frps.sh", "frps", "frps_enable", 0, SVC_FLAG_LATE_START, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_VLMCSD)
    { "vlmcsd", "/usr/bin/vlmcsd.sh", "vlmcsd", "vlmcsd_enable", 0, SVC_FLAG_SINGLETON, NULL, NULL, NULL, NULL, NULL },
#endif

#if defined(APP_SHADOWSOCKS)
    { "ss-redir", "/usr/bin/ss-redir.sh", "ss-redir", "ss_enable", 0, SVC_FLAG_RESTART_FW, NULL, NULL, NULL, NULL, NULL },
#endif

    /* Sentinel */
    { NULL, NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL }
};

/* Runtime state */
static struct service_entry *service_entries = NULL;
static int service_count_cache = 0;

void
service_shutdown(void)
{
    if (service_entries) {
        free(service_entries);
        service_entries = NULL;
    }

    service_count_cache = 0;
}

int
service_init(void)
{
    int count = 0;
    int i;
    const struct service_desc *desc;

    /* Count services */
    for (desc = service_table; desc->name; desc++)
        count++;

    service_shutdown();

    /* Allocate entries */
    service_count_cache = count;
    if (count <= 0)
        return 0;

    service_entries = calloc(count, sizeof(struct service_entry));
    if (!service_entries)
        return -1;

    /* Initialize entries */
    for (i = 0; i < count; i++) {
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
