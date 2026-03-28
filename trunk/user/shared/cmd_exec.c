/*
 * Safe Command Execution Implementation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "cmd_exec.h"
#include "pids.h"

/* Maximum iptables command length */
#define IPT_CMD_MAX 1024

void
ipt_rule_init(struct ipt_rule *rule)
{
    if (!rule)
        return;

    memset(rule, 0, sizeof(*rule));
}

/* Build iptables command string */
static int
build_ipt_cmd(char *cmd, size_t cmd_size, const char *cmd_type,
              const struct ipt_rule *rule, int ipv6)
{
    int len = 0;
    const char *ipt_cmd = ipv6 ? "ip6tables" : "iptables";

    len = snprintf(cmd, cmd_size, "%s ", ipt_cmd);

    if (rule->table)
        len += snprintf(cmd + len, cmd_size - len, "-t %s ", rule->table);

    len += snprintf(cmd + len, cmd_size - len, "%s %s ", cmd_type, rule->chain ? rule->chain : "INPUT");

    if (rule->proto)
        len += snprintf(cmd + len, cmd_size - len, "-p %s ", rule->proto);

    if (rule->src_ip)
        len += snprintf(cmd + len, cmd_size - len, "-s %s ", rule->src_ip);

    if (rule->dst_ip)
        len += snprintf(cmd + len, cmd_size - len, "-d %s ", rule->dst_ip);

    if (rule->src_port)
        len += snprintf(cmd + len, cmd_size - len, "--sport %s ", rule->src_port);

    if (rule->dst_port)
        len += snprintf(cmd + len, cmd_size - len, "--dport %s ", rule->dst_port);

    if (rule->in_iface)
        len += snprintf(cmd + len, cmd_size - len, "-i %s ", rule->in_iface);

    if (rule->out_iface)
        len += snprintf(cmd + len, cmd_size - len, "-o %s ", rule->out_iface);

    if (rule->match)
        len += snprintf(cmd + len, cmd_size - len, "-m %s ", rule->match);

    if (rule->action)
        len += snprintf(cmd + len, cmd_size - len, "-j %s ", rule->action);

    if (rule->extra)
        len += snprintf(cmd + len, cmd_size - len, "%s ", rule->extra);

    return (len < cmd_size) ? 0 : -1;
}

int
iptables_add_rule(const struct ipt_rule *rule)
{
    char cmd[IPT_CMD_MAX];

    if (!rule || !rule->chain)
        return -1;

    if (build_ipt_cmd(cmd, sizeof(cmd), "-A", rule, 0) < 0)
        return -1;

    return system(cmd);
}

int
iptables_accept_iface(const char *iface, const char *chain)
{
    struct ipt_rule rule;

    ipt_rule_init(&rule);
    rule.chain = chain;
    rule.in_iface = iface;
    rule.action = "ACCEPT";

    return iptables_add_rule(&rule);
}

int
iptables_drop_src(const char *src, const char *chain)
{
    struct ipt_rule rule;

    ipt_rule_init(&rule);
    rule.chain = chain;
    rule.src_ip = src;
    rule.action = "DROP";

    return iptables_add_rule(&rule);
}

int
iptables_accept_tcp_port(const char *port, const char *chain)
{
    struct ipt_rule rule;

    ipt_rule_init(&rule);
    rule.chain = chain;
    rule.proto = "tcp";
    rule.dst_port = port;
    rule.action = "ACCEPT";

    return iptables_add_rule(&rule);
}

int
iptables_dnat(const char *proto, const char *dport, const char *to_ip, const char *to_port)
{
    struct ipt_rule rule;
    char extra[128];

    if (!to_ip || !to_port)
        return -1;

    if (snprintf(extra, sizeof(extra), "--to-destination %s:%s", to_ip, to_port) >= (int)sizeof(extra))
        return -1;

    ipt_rule_init(&rule);
    rule.table = "nat";
    rule.chain = "PREROUTING";
    rule.proto = proto;
    rule.dst_port = dport;
    rule.action = "DNAT";
    rule.extra = extra;

    return iptables_add_rule(&rule);
}

int
iptables_masquerade(const char *iface)
{
    struct ipt_rule rule;

    ipt_rule_init(&rule);
    rule.table = "nat";
    rule.chain = "POSTROUTING";
    rule.out_iface = iface;
    rule.action = "MASQUERADE";

    return iptables_add_rule(&rule);
}

int
iptables_del_rule(const struct ipt_rule *rule)
{
    char cmd[IPT_CMD_MAX];

    if (!rule || !rule->chain)
        return -1;

    if (build_ipt_cmd(cmd, sizeof(cmd), "-D", rule, 0) < 0)
        return -1;

    return system(cmd);
}

int
iptables_flush_chain(const char *table, const char *chain)
{
    char cmd[256];
    int len;

    if (!chain)
        return -1;

    len = snprintf(cmd, sizeof(cmd), "iptables ");
    if (table)
        len += snprintf(cmd + len, sizeof(cmd) - len, "-t %s ", table);
    snprintf(cmd + len, sizeof(cmd) - len, "-F %s", chain);

    return system(cmd);
}

int
iptables_new_chain(const char *table, const char *chain)
{
    char cmd[256];
    int len;

    if (!chain)
        return -1;

    len = snprintf(cmd, sizeof(cmd), "iptables ");
    if (table)
        len += snprintf(cmd + len, sizeof(cmd) - len, "-t %s ", table);
    snprintf(cmd + len, sizeof(cmd) - len, "-N %s", chain);

    return system(cmd);
}

int
iptables_del_chain(const char *table, const char *chain)
{
    char cmd[256];
    int len;

    if (!chain)
        return -1;

    len = snprintf(cmd, sizeof(cmd), "iptables ");
    if (table)
        len += snprintf(cmd + len, sizeof(cmd) - len, "-t %s ", table);
    snprintf(cmd + len, sizeof(cmd) - len, "-X %s", chain);

    return system(cmd);
}

int
ip6tables_add_rule(const struct ip6t_rule *rule)
{
    char cmd[IPT_CMD_MAX];

    if (!rule || !rule->chain)
        return -1;

    if (build_ipt_cmd(cmd, sizeof(cmd), "-A", rule, 1) < 0)
        return -1;

    return system(cmd);
}

int
ip6tables_del_rule(const struct ip6t_rule *rule)
{
    char cmd[IPT_CMD_MAX];

    if (!rule || !rule->chain)
        return -1;

    if (build_ipt_cmd(cmd, sizeof(cmd), "-D", rule, 1) < 0)
        return -1;

    return system(cmd);
}

int
ip6tables_flush_chain(const char *table, const char *chain)
{
    char cmd[256];
    int len;

    if (!chain)
        return -1;

    len = snprintf(cmd, sizeof(cmd), "ip6tables ");
    if (table)
        len += snprintf(cmd + len, sizeof(cmd) - len, "-t %s ", table);
    snprintf(cmd + len, sizeof(cmd) - len, "-F %s", chain);

    return system(cmd);
}

/*
 * Process management implementation
 */

int
proc_signal(const char *name, int sig)
{
    pid_t *pids;
    pid_t *p;
    int count = 0;

    if (!name)
        return -1;

    pids = find_pid_by_name(name);
    if (!pids)
        return -1;

    for (p = pids; *p; p++) {
        if (kill(*p, sig) == 0)
            count++;
    }

    free(pids);
    return count;
}

int
proc_signal_wait(const char *name, int sig, int timeout_sec)
{
    int i;

    if (proc_signal(name, sig) < 0)
        return -1;

    for (i = 0; i < timeout_sec; i++) {
        if (!proc_is_running(name))
            return 0;
        sleep(1);
    }

    return -1;  /* Timeout */
}

int
proc_is_running(const char *name)
{
    return pids((char *)name);
}

int
proc_get_pid(const char *name)
{
    pid_t *pids;
    int result = -1;

    if (!name)
        return -1;

    pids = find_pid_by_name((char *)name);
    if (pids) {
        if (pids[0])
            result = pids[0];
        free(pids);
    }

    return result;
}

int
proc_get_pids(const char *name, int *pids_out, int max_pids)
{
    pid_t *pids_found;
    int count = 0;
    pid_t *p;

    if (!name || !pids_out || max_pids <= 0)
        return -1;

    pids_found = find_pid_by_name((char *)name);
    if (!pids_found)
        return 0;

    for (p = pids_found; *p && count < max_pids; p++)
        pids_out[count++] = *p;

    free(pids_found);
    return count;
}

/*
 * Service control implementation
 */

int
svc_start(const char *script)
{
    char cmd[256];

    if (!script)
        return -1;

    snprintf(cmd, sizeof(cmd), "%s start", script);
    return system(cmd);
}

int
svc_stop(const char *script)
{
    char cmd[256];

    if (!script)
        return -1;

    snprintf(cmd, sizeof(cmd), "%s stop", script);
    return system(cmd);
}

int
svc_restart(const char *script)
{
    if (svc_stop(script) != 0)
        return -1;

    sleep(1);  /* Wait for service to fully stop */

    return svc_start(script);
}
