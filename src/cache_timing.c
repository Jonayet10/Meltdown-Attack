#include "cache_timing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

const size_t REPEATS = 100000;

int main() {
    uint64_t sum_miss = 0;
    uint64_t sum_hit = 0;

    // TODO: Implement the algorithm as described in the specification here
    for (size_t i = 0; i < REPEATS; i++) {
        page_t *pages = calloc(UINT8_MAX + 1, PAGE_SIZE);
        assert(pages != NULL);
        flush_cache_line(pages);

        // flush_cache_line(pages) invalidates the cache line comprising the address
        // pointed to by pages. So the next read operation will not find the data in the
        // cache. When time_read is first called for miss_time, since the data was
        // eviceted from the cache from flush, this read has to fetch the data from main
        // memory, which takes longer, hence a cache miss.
        uint64_t miss_time = time_read(pages);

        // The second time time_read is called for hit_time, because the data was just
        // read from main memory and loaded into the cache from previous time_read call,
        // the data is now in the cache. So the CPU finds the data in the cache, and does
        // not need to access main memory, which is faster, hence a cache hit.
        uint64_t hit_time = time_read(pages);

        // Only count instances where the cache hit time is less than cache miss time,
        // because otherwise does not make sense.
        if (hit_time < miss_time) {
            sum_miss += miss_time;
            sum_hit += hit_time;
        }
    }

    printf("average miss = %" PRIu64 "\n", sum_miss / REPEATS);
    printf("average hit  = %" PRIu64 "\n", sum_hit / REPEATS);
}
