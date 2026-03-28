/*
 * Configuration Validation Implementation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <regex.h>
#include "config_validate.h"
#include "nvram_linux.h"

/*
 * Default configuration schema
 * This should be extended with product-specific configurations
 */
static const struct config_schema default_schema[] = {
    /* Network settings */
    CFG_IP("lan_ipaddr", "192.168.2.1", "LAN IP address"),
    CFG_IP("lan_netmask", "255.255.255.0", "LAN netmask"),
    CFG_STRING("lan_gateway", NULL, "", "LAN gateway"),
    CFG_STRING("lan_dns1", NULL, "", "Primary DNS server"),
    CFG_STRING("lan_dns2", NULL, "", "Secondary DNS server"),

    /* WAN settings */
    CFG_INT("wan_proto", "0", "3", "0", "WAN protocol (0=dhcp, 1=static, 2=pppoe, 3=disabled)"),
    CFG_IP("wan_ipaddr", "0.0.0.0", "WAN IP address"),
    CFG_IP("wan_netmask", "0.0.0.0", "WAN netmask"),
    CFG_IP("wan_gateway", "0.0.0.0", "WAN gateway"),

    /* HTTP settings */
    CFG_INT("http_lanport", "1", "65535", "80", "HTTP LAN port"),
    CFG_INT("https_lport", "1", "65535", "443", "HTTPS LAN port"),
    CFG_INT("http_access", "0", "2", "0", "HTTP access mode"),

    /* WiFi settings */
    CFG_STRING("wl_ssid", NULL, "", "5GHz WiFi SSID"),
    CFG_STRING("rt_ssid", NULL, "", "2.4GHz WiFi SSID"),
    CFG_INT("wl_channel", "0", "165", "0", "5GHz WiFi channel"),
    CFG_INT("rt_channel", "0", "13", "0", "2.4GHz WiFi channel"),

    /* Firewall */
    CFG_INT("fw_enable_x", "0", "1", "1", "Firewall enable"),

    /* Sentinel */
    { NULL, CFG_TYPE_INT, NULL, NULL, NULL, NULL, NULL, 0 }
};

static const struct config_schema *custom_schema = NULL;
static int custom_schema_count = 0;

int
config_validate_init(void)
{
    return 0;
}

int
config_is_valid_ip(const char *ip)
{
    struct in_addr addr;

    if (!ip || !ip[0])
        return 0;

    return inet_pton(AF_INET, ip, &addr) == 1;
}

int
config_is_valid_ipv6(const char *ip)
{
    struct in6_addr addr;

    if (!ip || !ip[0])
        return 0;

    return inet_pton(AF_INET6, ip, &addr) == 1;
}

int
config_is_valid_mac(const char *mac)
{
    int i;
    unsigned int val;

    if (!mac || !mac[0])
        return 0;

    /* Check format: XX:XX:XX:XX:XX:XX */
    if (strlen(mac) != 17)
        return 0;

    for (i = 0; i < 6; i++) {
        if (i > 0 && mac[i * 3 - 1] != ':')
            return 0;
        if (sscanf(mac + i * 3, "%02x", &val) != 1)
            return 0;
        if (val > 0xFF)
            return 0;
    }

    return 1;
}

int
config_is_valid_port(const char *port)
{
    int p;

    if (!port || !port[0])
        return 0;

    p = atoi(port);
    return (p >= 1 && p <= 65535);
}

int
config_is_valid_netmask(const char *mask)
{
    struct in_addr addr;
    unsigned long m;
    int bits;

    if (!mask || !mask[0])
        return 0;

    if (!inet_pton(AF_INET, mask, &addr))
        return 0;

    m = ntohl(addr.s_addr);

    /* Check if it's a valid netmask */
    bits = 0;
    while (m & 0x80000000) {
        bits++;
        m <<= 1;
    }

    /* All remaining bits should be 0 */
    if (m != 0)
        return 0;

    return (bits >= 0 && bits <= 32);
}

int
config_is_valid_hostname(const char *hostname)
{
    const char *p;

    if (!hostname || !hostname[0])
        return 0;

    if (strlen(hostname) > 63)
        return 0;

    /* Must start with alphanumeric */
    if (!isalnum(hostname[0]))
        return 0;

    /* Must end with alphanumeric */
    if (!isalnum(hostname[strlen(hostname) - 1]))
        return 0;

    /* Can only contain alphanumeric and hyphen */
    for (p = hostname; *p; p++) {
        if (!isalnum(*p) && *p != '-')
            return 0;
    }

    return 1;
}

int
config_is_valid_ssid(const char *ssid)
{
    if (!ssid || !ssid[0])
        return 0;

    if (strlen(ssid) > 32)
        return 0;

    return 1;
}

int
config_is_valid_wpa_key(const char *key)
{
    int len;
    int i;

    if (!key || !key[0])
        return 0;

    len = strlen(key);

    /* WPA-PSK: 8-63 characters or 64 hex digits */
    if (len == 64) {
        /* Check if all hex */
        for (i = 0; i < 64; i++) {
            if (!isxdigit(key[i]))
                return 0;
        }
        return 1;
    }

    return (len >= 8 && len <= 63);
}

static int
validate_int(const char *value, const struct config_schema *schema)
{
    int val, min_val = 0, max_val = 0;

    if (!value || !value[0])
        return 0;

    val = atoi(value);

    if (schema->min_val)
        min_val = atoi(schema->min_val);

    if (schema->max_val)
        max_val = atoi(schema->max_val);

    if (schema->min_val && schema->max_val)
        return (val >= min_val && val <= max_val);

    if (schema->min_val)
        return (val >= min_val);

    if (schema->max_val)
        return (val <= max_val);

    return 1;
}

static int
validate_string(const char *value, const struct config_schema *schema)
{
    regex_t regex;
    int ret;

    if (!schema->pattern)
        return 1;

    if (!value)
        value = "";

    if (regcomp(&regex, schema->pattern, REG_EXTENDED | REG_NOSUB) != 0)
        return 1;

    ret = regexec(&regex, value, 0, NULL, 0);
    regfree(&regex);

    return (ret == 0);
}

const struct config_schema *
config_get_schema(const char *key)
{
    const struct config_schema *s;

    /* Check custom schema first */
    if (custom_schema) {
        for (s = custom_schema; s->key; s++) {
            if (strcmp(s->key, key) == 0)
                return s;
        }
    }

    /* Check default schema */
    for (s = default_schema; s->key; s++) {
        if (strcmp(s->key, key) == 0)
            return s;
    }

    return NULL;
}

int
config_validate_one(const char *key, const char *value, struct config_result *result)
{
    const struct config_schema *schema;
    const char *actual_value;
    int valid = 1;

    schema = config_get_schema(key);
    if (!schema) {
        if (result) {
            result->key = key;
            result->valid = 1;  /* Unknown keys are considered valid */
            result->error_msg = NULL;
        }
        return 1;
    }

    actual_value = value ? value : nvram_get(key);

    /* Check required */
    if ((schema->flags & CFG_FLAG_REQUIRED) && (!actual_value || !actual_value[0])) {
        if (result) {
            result->key = key;
            result->valid = 0;
            result->error_msg = "Required value is missing";
        }
        return 0;
    }

    /* Validate by type */
    switch (schema->type) {
        case CFG_TYPE_INT:
            valid = validate_int(actual_value, schema);
            break;

        case CFG_TYPE_STRING:
            valid = validate_string(actual_value, schema);
            break;

        case CFG_TYPE_IP:
            valid = config_is_valid_ip(actual_value);
            break;

        case CFG_TYPE_MAC:
            valid = config_is_valid_mac(actual_value);
            break;

        case CFG_TYPE_PORT:
            valid = config_is_valid_port(actual_value);
            break;

        case CFG_TYPE_NETMASK:
            valid = config_is_valid_netmask(actual_value);
            break;

        case CFG_TYPE_BOOL:
            valid = (actual_value && (strcmp(actual_value, "0") == 0 ||
                                       strcmp(actual_value, "1") == 0));
            break;

        default:
            valid = 1;
            break;
    }

    if (result) {
        result->key = key;
        result->valid = valid;
        result->error_msg = valid ? NULL : "Invalid value";
    }

    return valid;
}

int
config_validate_all(struct config_result *results, int max_results)
{
    const struct config_schema *s;
    int invalid_count = 0;
    int result_idx = 0;

    /* Validate custom schema */
    if (custom_schema) {
        for (s = custom_schema; s->key; s++) {
            struct config_result res;
            if (!config_validate_one(s->key, NULL, &res)) {
                invalid_count++;
                if (results && result_idx < max_results)
                    results[result_idx++] = res;
            }
        }
    }

    /* Validate default schema */
    for (s = default_schema; s->key; s++) {
        struct config_result res;
        if (!config_validate_one(s->key, NULL, &res)) {
            invalid_count++;
            if (results && result_idx < max_results)
                results[result_idx++] = res;
        }
    }

    return invalid_count;
}

int
config_apply_defaults(void)
{
    const struct config_schema *s;

    /* Apply custom schema defaults */
    if (custom_schema) {
        for (s = custom_schema; s->key; s++) {
            if (!nvram_get(s->key) && s->default_val)
                nvram_set(s->key, s->default_val);
        }
    }

    /* Apply default schema defaults */
    for (s = default_schema; s->key; s++) {
        if (!nvram_get(s->key) && s->default_val)
            nvram_set(s->key, s->default_val);
    }

    return 0;
}

int
config_reset_default(const char *key)
{
    const struct config_schema *s = config_get_schema(key);

    if (!s)
        return -1;

    if (s->default_val)
        nvram_set(key, s->default_val);
    else
        nvram_unset(key);

    return 0;
}

int
config_register_schema(const struct config_schema *schema)
{
    custom_schema = schema;
    return 0;
}
