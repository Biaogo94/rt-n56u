/*
 * NVRAM Hot Cache Implementation - Stability Enhanced Version
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "nvram_cache.h"
#include "nvram_linux.h"

/*
 * Stability: Disable pthread mutex for single-threaded embedded environment
 * The router's main processes are typically single-threaded for NVRAM access
 * This reduces complexity and avoids potential deadlock issues
 */
#define USE_THREAD_SAFETY 0

#if USE_THREAD_SAFETY
#include <pthread.h>
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#define CACHE_LOCK() pthread_mutex_lock(&cache_mutex)
#define CACHE_UNLOCK() pthread_mutex_unlock(&cache_mutex)
#else
#define CACHE_LOCK() do {} while(0)
#define CACHE_UNLOCK() do {} while(0)
#endif

/*
 * Cache entry structure
 */
struct nvram_cache_entry {
    char key[NVRAM_CACHE_KEY_MAX];
    char value[NVRAM_CACHE_VALUE_MAX];
    unsigned int timestamp;  /* Creation time in seconds */
    unsigned int hits;
    unsigned int hash;
    unsigned char valid;     /* Use unsigned char for alignment */
    unsigned char ttl_multi; /* TTL multiplier for extended TTL */
};

/*
 * Static cache storage - no dynamic allocation
 */
static struct nvram_cache_entry cache_entries[NVRAM_CACHE_SIZE];

/*
 * Cache configuration
 */
static unsigned int cache_ttl = NVRAM_CACHE_TTL_DEFAULT;
static int cache_enabled = 1;
static int cache_initialized = 0;

/*
 * Statistics and error tracking
 */
static struct nvram_cache_stats cache_stats;

/*
 * Error tracking for auto-disable
 */
#define MAX_CONSECUTIVE_ERRORS 5
static int consecutive_errors = 0;
static int total_errors = 0;

/*
 * Frequently accessed NVRAM keys to prefetch
 * These are commonly read during Web UI requests and status polling
 * Sorted by access frequency (most frequent first)
 */
const char *nvram_hotkeys[] = {
    /* High frequency - network status (polled every few seconds) */
    "wan_ipaddr",
    "wan_netmask",
    "wan_gateway",
    "wan_proto",
    "lan_ipaddr",
    "lan_netmask",

    /* WiFi status - frequently accessed */
    "wl_ssid",
    "rt_ssid",
    "wl_radio_x",
    "rt_radio_x",
    "wl_channel",
    "rt_channel",

    /* Service enables - checked frequently */
    "sshd_enable",
    "crond_enable",
    "ddns_enable_x",
    "fw_enable_x",

    /* DHCP */
    "dhcp_enable_x",
    "dhcp_start",
    "dhcp_end",

    /* System */
    "http_lanport",
    "sw_mode",
    "time_zone",

    /* WiFi security */
    "wl_auth_mode",
    "rt_auth_mode",
    "wl_crypto",
    "rt_crypto",

    NULL  /* Sentinel */
};

/*
 * Simple hash function (FNV-1a variant - better distribution than djb2)
 */
static unsigned int cache_hash(const char *str)
{
    unsigned int hash = 2166136261u;
    int c;

    while ((c = *str++)) {
        hash ^= (unsigned char)c;
        hash *= 16777619u;
    }

    return hash & (NVRAM_CACHE_SIZE - 1);
}

/*
 * Get current time in seconds (more portable than milliseconds)
 */
static unsigned int get_time_sec(void)
{
    return (unsigned int)time(NULL);
}

/*
 * Check if entry is expired
 */
static int cache_entry_expired(struct nvram_cache_entry *entry)
{
    if (cache_ttl == 0)
        return 1;

    unsigned int now = get_time_sec();
    unsigned int elapsed = now - entry->timestamp;
    unsigned int effective_ttl = cache_ttl / 1000; /* Convert ms to sec */

    /* Use TTL multiplier for entries with many hits */
    if (entry->ttl_multi > 0 && effective_ttl > 0) {
        effective_ttl *= (1 + (entry->hits / 100)); /* Extend TTL for hot entries */
        if (effective_ttl > 60) /* Cap at 60 seconds */
            effective_ttl = 60;
    }

    return elapsed > effective_ttl;
}

/*
 * Find cache entry by key (linear probe for collision resolution)
 */
static struct nvram_cache_entry *cache_find_entry(const char *key)
{
    unsigned int hash = cache_hash(key);
    unsigned int idx = hash;
    unsigned int attempts = 0;

    while (attempts < NVRAM_CACHE_SIZE) {
        struct nvram_cache_entry *entry = &cache_entries[idx];

        if (!entry->valid)
            return NULL;

        if (entry->hash == hash && strcmp(entry->key, key) == 0)
            return entry;

        idx = (idx + 1) & (NVRAM_CACHE_SIZE - 1);
        attempts++;
    }

    return NULL;
}

/*
 * Find an empty or expired slot
 */
static struct nvram_cache_entry *cache_find_slot(unsigned int hash)
{
    unsigned int idx = hash;
    unsigned int attempts = 0;
    struct nvram_cache_entry *expired = NULL;
    struct nvram_cache_entry *oldest = NULL;
    unsigned int oldest_time = 0xFFFFFFFF;

    while (attempts < NVRAM_CACHE_SIZE) {
        struct nvram_cache_entry *entry = &cache_entries[idx];

        /* Empty slot found */
        if (!entry->valid)
            return entry;

        /* Track expired entry */
        if (cache_entry_expired(entry)) {
            if (!expired) {
                expired = entry;
            }
        }

        /* Track oldest entry for potential eviction */
        if (entry->timestamp < oldest_time) {
            oldest_time = entry->timestamp;
            oldest = entry;
        }

        idx = (idx + 1) & (NVRAM_CACHE_SIZE - 1);
        attempts++;
    }

    /* Prefer expired entry */
    if (expired) {
        cache_stats.evictions++;
        return expired;
    }

    /* Evict oldest entry */
    if (oldest) {
        cache_stats.evictions++;
        return oldest;
    }

    /* Should never reach here */
    return &cache_entries[0];
}

/*
 * Record an error for auto-disable mechanism
 */
static void record_cache_error(void)
{
    consecutive_errors++;
    total_errors++;

    if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
        cache_enabled = 0;
        /* Log: Cache disabled due to consecutive errors */
    }
}

/*
 * Clear error counter on success
 */
static void clear_cache_error(void)
{
    consecutive_errors = 0;
}

int
nvram_cache_init(void)
{
    if (cache_initialized)
        return 0;

    CACHE_LOCK();

    memset(cache_entries, 0, sizeof(cache_entries));
    memset(&cache_stats, 0, sizeof(cache_stats));
    consecutive_errors = 0;
    total_errors = 0;
    cache_initialized = 1;

    CACHE_UNLOCK();

    return 0;
}

void
nvram_cache_shutdown(void)
{
    CACHE_LOCK();

    memset(cache_entries, 0, sizeof(cache_entries));
    cache_initialized = 0;

    CACHE_UNLOCK();
}

char *
nvram_cache_get(const char *name)
{
    /* Safety checks - fallback to direct NVRAM */
    if (!name || !name[0])
        return NULL;

    /* Auto-disable check */
    if (!cache_enabled || !cache_initialized)
        return nvram_get_(name);

    /* Key too long - don't cache */
    if (strlen(name) >= NVRAM_CACHE_KEY_MAX)
        return nvram_get_(name);

    CACHE_LOCK();

    struct nvram_cache_entry *entry = cache_find_entry(name);

    if (entry && !cache_entry_expired(entry)) {
        /* Cache hit */
        entry->hits++;
        cache_stats.hits++;
        clear_cache_error();
        CACHE_UNLOCK();
        return entry->value;
    }

    /* Cache miss */
    cache_stats.misses++;
    CACHE_UNLOCK();

    /* Get from NVRAM */
    char *value = nvram_get_(name);
    if (!value)
        return NULL;

    /* Value too long - don't cache */
    if (strlen(value) >= NVRAM_CACHE_VALUE_MAX)
        return value;

    /* Update cache */
    nvram_cache_update(name, value);

    /* Return direct NVRAM value to ensure consistency */
    return value;
}

int
nvram_cache_get_int(const char *name)
{
    char *value = nvram_cache_get(name);
    return value ? atoi(value) : 0;
}

void
nvram_cache_update(const char *name, const char *value)
{
    /* Safety checks */
    if (!name || !name[0])
        return;

    if (!cache_enabled || !cache_initialized)
        return;

    size_t key_len = strlen(name);
    size_t value_len = value ? strlen(value) : 0;

    if (key_len >= NVRAM_CACHE_KEY_MAX)
        return;

    if (value && value_len >= NVRAM_CACHE_VALUE_MAX)
        return;

    CACHE_LOCK();

    unsigned int hash = cache_hash(name);
    struct nvram_cache_entry *entry = cache_find_entry(name);

    if (entry) {
        /* Update existing entry */
        if (value) {
            memcpy(entry->value, value, value_len + 1);
            entry->timestamp = get_time_sec();
            entry->valid = 1;
        } else {
            /* Invalidate entry */
            entry->valid = 0;
            cache_stats.invalidations++;
        }
    } else if (value) {
        /* Create new entry */
        entry = cache_find_slot(hash);
        if (entry) {
            entry->hash = hash;
            memcpy(entry->key, name, key_len + 1);
            memcpy(entry->value, value, value_len + 1);
            entry->timestamp = get_time_sec();
            entry->hits = 0;
            entry->ttl_multi = 0;
            entry->valid = 1;
        }
    }

    CACHE_UNLOCK();
}

void
nvram_cache_invalidate(const char *name)
{
    CACHE_LOCK();

    if (name) {
        /* Invalidate specific key */
        struct nvram_cache_entry *entry = cache_find_entry(name);
        if (entry) {
            entry->valid = 0;
            cache_stats.invalidations++;
        }
    } else {
        /* Invalidate all */
        for (unsigned int i = 0; i < NVRAM_CACHE_SIZE; i++) {
            cache_entries[i].valid = 0;
        }
        cache_stats.invalidations += NVRAM_CACHE_SIZE;
    }

    CACHE_UNLOCK();
}

void
nvram_cache_set_ttl(unsigned int ttl_ms)
{
    CACHE_LOCK();
    /* Minimum TTL of 1 second (1000ms), maximum 60 seconds */
    if (ttl_ms > 0 && ttl_ms < 1000)
        ttl_ms = 1000;
    if (ttl_ms > 60000)
        ttl_ms = 60000;
    cache_ttl = ttl_ms;
    CACHE_UNLOCK();
}

void
nvram_cache_enable(int enable)
{
    CACHE_LOCK();
    cache_enabled = enable;
    if (!enable) {
        memset(cache_entries, 0, sizeof(cache_entries));
    }
    consecutive_errors = 0; /* Reset error counter */
    CACHE_UNLOCK();
}

void
nvram_cache_get_stats(struct nvram_cache_stats *stats)
{
    if (stats) {
        CACHE_LOCK();
        *stats = cache_stats;
        CACHE_UNLOCK();
    }
}

void
nvram_cache_prefetch_hotkeys(void)
{
    const char **key;

    if (!cache_enabled || !cache_initialized)
        return;

    for (key = nvram_hotkeys; *key; key++) {
        char *value = nvram_get_(*key);
        if (value && strlen(value) < NVRAM_CACHE_VALUE_MAX) {
            nvram_cache_update(*key, value);
        }
    }
}

/*
 * Debug function - get cache efficiency percentage
 */
int nvram_cache_get_efficiency(void)
{
    CACHE_LOCK();
    unsigned long total = cache_stats.hits + cache_stats.misses;
    int efficiency = total > 0 ? (int)((cache_stats.hits * 100) / total) : 0;
    CACHE_UNLOCK();
    return efficiency;
}

/*
 * Check cache health - for monitoring
 */
int nvram_cache_is_healthy(void)
{
    return cache_enabled && (consecutive_errors < MAX_CONSECUTIVE_ERRORS);
}
