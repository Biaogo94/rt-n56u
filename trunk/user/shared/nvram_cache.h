/*
 * NVRAM Hot Cache - Performance optimization layer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Features:
 * - Hash table based cache for frequently accessed NVRAM keys
 * - TTL-based cache invalidation with automatic extension for hot keys
 * - Minimal memory footprint for embedded systems
 * - Auto-disable on consecutive errors for stability
 * - Thread-safe option for multi-threaded environments
 */

#ifndef _NVRAM_CACHE_H_
#define _NVRAM_CACHE_H_

#include <stddef.h>

/*
 * Cache configuration
 */
#define NVRAM_CACHE_SIZE        64      /* Number of cache slots (power of 2) */
#define NVRAM_CACHE_TTL_DEFAULT 5000    /* Default TTL in milliseconds (5 seconds) */
#define NVRAM_CACHE_KEY_MAX     64      /* Maximum key length */
#define NVRAM_CACHE_VALUE_MAX   256     /* Maximum cached value length */

/*
 * Cache statistics
 */
struct nvram_cache_stats {
    unsigned long hits;
    unsigned long misses;
    unsigned long evictions;
    unsigned long invalidations;
};

/*
 * Initialize the NVRAM cache
 * @return: 0 on success, -1 on error
 */
int nvram_cache_init(void);

/*
 * Shutdown the NVRAM cache
 */
void nvram_cache_shutdown(void);

/*
 * Get value from cache (with fallback to NVRAM)
 * @param name: NVRAM key name
 * @return: Cached value or NULL (always returns valid NVRAM data)
 *
 * NOTE: The returned pointer may point to cache memory OR NVRAM memory.
 *       Do NOT modify the returned string. Copy it if modification needed.
 */
char *nvram_cache_get(const char *name);

/*
 * Get integer value from cache
 * @param name: NVRAM key name
 * @return: Integer value or 0 if not found
 */
int nvram_cache_get_int(const char *name);

/*
 * Update cache entry (called automatically by nvram_set via nvram_cache_sync)
 * @param name: NVRAM key name
 * @param value: New value (NULL = invalidate)
 */
void nvram_cache_update(const char *name, const char *value);

/*
 * Invalidate a cache entry
 * @param name: NVRAM key name (NULL = invalidate all)
 */
void nvram_cache_invalidate(const char *name);

/*
 * Set TTL for cache entries
 * @param ttl_ms: Time-to-live in milliseconds
 *                0 = disable caching
 *                Min: 1000 (1 second), Max: 60000 (60 seconds)
 */
void nvram_cache_set_ttl(unsigned int ttl_ms);

/*
 * Enable/disable caching
 * @param enable: 1 to enable, 0 to disable
 */
void nvram_cache_enable(int enable);

/*
 * Get cache statistics
 * @param stats: Output statistics structure
 */
void nvram_cache_get_stats(struct nvram_cache_stats *stats);

/*
 * Prefetch hot keys into cache
 * Call this during system initialization (after NVRAM is ready)
 */
void nvram_cache_prefetch_hotkeys(void);

/*
 * Get cache efficiency percentage
 * @return: 0-100 percent hit rate
 */
int nvram_cache_get_efficiency(void);

/*
 * Check if cache is healthy
 * @return: 1 if healthy, 0 if disabled due to errors
 */
int nvram_cache_is_healthy(void);

/*
 * List of frequently accessed keys to prefetch
 */
extern const char *nvram_hotkeys[];

#endif /* _NVRAM_CACHE_H_ */
