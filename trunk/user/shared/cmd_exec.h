/*
 * Safe Command Execution API
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

#ifndef _CMD_EXEC_H_
#define _CMD_EXEC_H_

#include <stddef.h>

/*
 * iptables rule structure for safe rule management
 */
struct ipt_rule {
    const char *table;      /* filter, nat, mangle, raw (NULL = filter) */
    const char *chain;      /* INPUT, OUTPUT, FORWARD, PREROUTING, POSTROUTING... */
    const char *action;     /* ACCEPT, DROP, REJECT, LOG, RETURN, DNAT, SNAT, MASQUERADE... */
    const char *proto;      /* tcp, udp, icmp, all (NULL = all) */
    const char *src_ip;     /* Source IP/CIDR (NULL = any) */
    const char *dst_ip;     /* Destination IP/CIDR (NULL = any) */
    const char *src_port;   /* Source port (NULL = any) */
    const char *dst_port;   /* Destination port (NULL = any) */
    const char *in_iface;   /* Input interface (NULL = any) */
    const char *out_iface;  /* Output interface (NULL = any) */
    const char *match;      /* Extra match modules (NULL = none) */
    const char *extra;      /* Extra options (NULL = none) */
};

/*
 * Initialize an iptables rule with safe defaults.
 */
void ipt_rule_init(struct ipt_rule *rule);

/*
 * ip6tables rule structure (same fields as ipt_rule)
 */
#define ip6t_rule ipt_rule

/*
 * iptables_add_rule - Add an iptables rule
 * @rule: Pointer to ipt_rule structure
 * @return: 0 on success, non-zero on failure
 */
int iptables_add_rule(const struct ipt_rule *rule);

/*
 * iptables_del_rule - Delete an iptables rule
 * @rule: Pointer to ipt_rule structure
 * @return: 0 on success, non-zero on failure
 */
int iptables_del_rule(const struct ipt_rule *rule);

/*
 * iptables_flush_chain - Flush all rules in a chain
 * @table: Table name (NULL = filter)
 * @chain: Chain name
 * @return: 0 on success, non-zero on failure
 */
int iptables_flush_chain(const char *table, const char *chain);

/*
 * iptables_new_chain - Create a new chain
 * @table: Table name (NULL = filter)
 * @chain: Chain name
 * @return: 0 on success, non-zero on failure
 */
int iptables_new_chain(const char *table, const char *chain);

/*
 * iptables_del_chain - Delete a chain
 * @table: Table name (NULL = filter)
 * @chain: Chain name
 * @return: 0 on success, non-zero on failure
 */
int iptables_del_chain(const char *table, const char *chain);

/*
 * ip6tables_add_rule - Add an ip6tables rule
 */
int ip6tables_add_rule(const struct ip6t_rule *rule);

/*
 * ip6tables_del_rule - Delete an ip6tables rule
 */
int ip6tables_del_rule(const struct ip6t_rule *rule);

/*
 * ip6tables_flush_chain - Flush all rules in a chain
 */
int ip6tables_flush_chain(const char *table, const char *chain);

/*
 * Process management functions
 */

/*
 * proc_signal - Send signal to a process by name
 * @name: Process name
 * @sig: Signal number (e.g., SIGTERM, SIGKILL, SIGUSR1)
 * @return: Number of processes signaled, -1 on error
 */
int proc_signal(const char *name, int sig);

/*
 * proc_signal_wait - Send signal and wait for termination
 * @name: Process name
 * @sig: Signal number
 * @timeout_sec: Maximum wait time in seconds
 * @return: 0 if terminated, -1 on error or timeout
 */
int proc_signal_wait(const char *name, int sig, int timeout_sec);

/*
 * proc_is_running - Check if a process is running
 * @name: Process name
 * @return: 1 if running, 0 if not running
 */
int proc_is_running(const char *name);

/*
 * proc_get_pid - Get PID of a process by name
 * @name: Process name
 * @return: PID if found, -1 if not found
 */
int proc_get_pid(const char *name);

/*
 * proc_get_pids - Get all PIDs of processes by name
 * @name: Process name
 * @pids: Array to store PIDs
 * @max_pids: Maximum number of PIDs to store
 * @return: Number of PIDs found
 */
int proc_get_pids(const char *name, int *pids, int max_pids);

/*
 * Service control functions
 */

/*
 * svc_start - Start a service by script
 * @script: Path to the start script
 * @return: Exit status of the script
 */
int svc_start(const char *script);

/*
 * svc_stop - Stop a service by script
 * @script: Path to the stop script
 * @return: Exit status of the script
 */
int svc_stop(const char *script);

/*
 * svc_restart - Restart a service
 * @script: Path to the service script (will be called with "stop" then "start")
 * @return: 0 on success, non-zero on failure
 */
int svc_restart(const char *script);

/*
 * Convenience helpers for common iptables operations.
 */
int iptables_accept_iface(const char *iface, const char *chain);
int iptables_drop_src(const char *src, const char *chain);
int iptables_accept_tcp_port(const char *port, const char *chain);
int iptables_dnat(const char *proto, const char *dport, const char *to_ip, const char *to_port);
int iptables_masquerade(const char *iface);

#endif /* _CMD_EXEC_H_ */
