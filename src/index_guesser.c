#include "index_guesser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

const size_t MIN_CHOICE = 1;
const size_t MAX_CHOICE = 255;

static inline page_t *init_pages(void) {
    page_t *pages = calloc(UINT8_MAX + 1, PAGE_SIZE);
    assert(pages != NULL);
    return pages;
}

static inline void flush_all_pages(page_t *pages) {
    for (size_t i = MIN_CHOICE; i <= MAX_CHOICE; i++) {
        flush_cache_line(&pages[i]);
    }
}

static inline size_t guess_accessed_page(page_t *pages) {
    // Threshold to distinguish between cache hit and miss was chosen to be 250.
    // Under 250 = cache hit.
    // Would have selected lower thresh, closer to 63 (highest hit time recorded),
    // to avoid false positives, but had to account for CPU doing other things.
    uint64_t thresh = 250;
    for (size_t i = MIN_CHOICE; i <= MAX_CHOICE; i++) {
        if ((time_read(&pages[i]) < thresh) && (time_read(&pages[i]) < thresh)) {
            return i;
        }
    }
    return 0;
}

int main() {
    page_t *pages = init_pages();

    flush_all_pages(pages);

    do_access(pages);

    size_t guess = guess_accessed_page(pages);
    if (guess > 0) {
        printf("%zu\n", guess);
    } else {
        printf("No page was accessed\n");
    }

    free(pages);
}
