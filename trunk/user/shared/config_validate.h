/*
 * Configuration Validation Framework
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _CONFIG_VALIDATE_H_
#define _CONFIG_VALIDATE_H_

#include <stddef.h>

/*
 * Configuration value types
 */
typedef enum {
    CFG_TYPE_INT,
    CFG_TYPE_STRING,
    CFG_TYPE_IP,
    CFG_TYPE_MAC,
    CFG_TYPE_PORT,
    CFG_TYPE_NETMASK,
    CFG_TYPE_BOOL,
    CFG_TYPE_RANGE_INT,
} config_type_t;

/*
 * Configuration schema entry
 */
struct config_schema {
    const char *key;            /* NVRAM key name */
    config_type_t type;         /* Value type */
    const char *min_val;        /* Min value for int/range types */
    const char *max_val;        /* Max value for int/range types */
    const char *pattern;        /* Regex pattern for string types */
    const char *default_val;    /* Default value */
    const char *description;    /* Human-readable description */
    int flags;                  /* Additional flags */
};

/* Configuration flags */
#define CFG_FLAG_REQUIRED    0x01  /* Value is required */
#define CFG_FLAG_READONLY    0x02  /* Cannot be changed at runtime */
#define CFG_FLAG_REBOOT      0x04  /* Requires reboot to take effect */

/*
 * Validation result
 */
struct config_result {
    const char *key;
    int valid;                  /* 1 = valid, 0 = invalid */
    const char *error_msg;      /* Error message if invalid */
    char corrected_val[256];    /* Corrected value if auto-corrected */
};

/*
 * Initialize config validation
 */
int config_validate_init(void);

/*
 * Validate a single configuration value
 * @param key: Configuration key
 * @param value: Value to validate (NULL = use NVRAM value)
 * @param result: Output result structure
 * @return: 1 if valid, 0 if invalid
 */
int config_validate_one(const char *key, const char *value, struct config_result *result);

/*
 * Validate all configurations against schema
 * @param results: Array to store results (can be NULL)
 * @param max_results: Size of results array
 * @return: Number of invalid configurations
 */
int config_validate_all(struct config_result *results, int max_results);

/*
 * Apply default values for missing/invalid configurations
 */
int config_apply_defaults(void);

/*
 * Reset configuration to default value
 * @param key: Configuration key
 * @return: 0 on success, -1 if key not found
 */
int config_reset_default(const char *key);

/*
 * Get configuration schema entry
 */
const struct config_schema *config_get_schema(const char *key);

/*
 * Register custom configuration schema
 * @param schema: Array of config_schema entries (NULL-terminated)
 * @return: 0 on success, -1 on error
 */
int config_register_schema(const struct config_schema *schema);

/*
 * Validate IP address format
 */
int config_is_valid_ip(const char *ip);

/*
 * Validate IPv6 address format
 */
int config_is_valid_ipv6(const char *ip);

/*
 * Validate MAC address format
 */
int config_is_valid_mac(const char *mac);

/*
 * Validate port number (1-65535)
 */
int config_is_valid_port(const char *port);

/*
 * Validate netmask format
 */
int config_is_valid_netmask(const char *mask);

/*
 * Validate hostname format
 */
int config_is_valid_hostname(const char *hostname);

/*
 * Validate SSID format
 */
int config_is_valid_ssid(const char *ssid);

/*
 * Validate WPA key format
 */
int config_is_valid_wpa_key(const char *key);

/*
 * Helper macros for defining config schema
 */

/* Integer config with range */
#define CFG_INT(key, min, max, def, desc) \
    { .key = key, .type = CFG_TYPE_INT, .min_val = min, .max_val = max, \
      .default_val = def, .description = desc }

/* String config with pattern */
#define CFG_STRING(key, pat, def, desc) \
    { .key = key, .type = CFG_TYPE_STRING, .pattern = pat, \
      .default_val = def, .description = desc }

/* IP address config */
#define CFG_IP(key, def, desc) \
    { .key = key, .type = CFG_TYPE_IP, .default_val = def, .description = desc }

/* MAC address config */
#define CFG_MAC(key, def, desc) \
    { .key = key, .type = CFG_TYPE_MAC, .default_val = def, .description = desc }

/* Port config */
#define CFG_PORT(key, def, desc) \
    { .key = key, .type = CFG_TYPE_PORT, .default_val = def, .description = desc }

/* Boolean config (0/1) */
#define CFG_BOOL(key, def, desc) \
    { .key = key, .type = CFG_TYPE_BOOL, .default_val = def, .description = desc }

/* Required config */
#define CFG_REQUIRED(schema) \
    { .key = schema.key, .type = schema.type, .min_val = schema.min_val, \
      .max_val = schema.max_val, .default_val = schema.default_val, \
      .description = schema.description, .flags = CFG_FLAG_REQUIRED }

#endif /* _CONFIG_VALIDATE_H_ */
