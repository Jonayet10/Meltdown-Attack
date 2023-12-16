#include "recover_local_secret.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

const size_t MIN_CHOICE = 'A' - 1;
const size_t MAX_CHOICE = 'Z' + 1;
const size_t SECRET_LENGTH = 5;

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
    uint64_t thresh = 250;
    for (size_t i = MIN_CHOICE; i <= MAX_CHOICE; i++) {
        if ((time_read(&pages[i]) < thresh) && (time_read(&pages[i]) < thresh)) {
            return i;
        }
    }
    return 0;
}

static inline void do_access(page_t *pages, size_t secret_index) {
    // Use access_secret(i) to get the ith character of the secret string.
    // Then use this character as an index into the page_t array. For example, if
    // the character is 'A' (which is equivalent to 65), index 65 of the page_t
    // array should be accessed. Make sure to use force_read() to ensure the
    // address is accessed.
    force_read(pages[access_secret(secret_index)]);
}

int main() {
    page_t *pages = init_pages();
    // Iterates once for each character in secret string/password. Loop
    // recovers one character in secret string per iteration.
    for (size_t i = 0; i < SECRET_LENGTH; i++) {
        // Before attempting to recover a character, need to ensure the CPU
        // cache does not contain previous operations that would interfere
        // with timing measurements.
        flush_all_pages(pages);
        // Access the page indexed by the next character in the secret string.
        // Loads the page into the cache.
        do_access(pages, i);
        // After do_access, one page corresponding to secret character is on
        // cache, and guess_accessed_page checks to see if it is a cache hit
        char guess = (char) guess_accessed_page(pages);
        // If it is a cache hit.
        if (guess > 0) {
            printf("%c", guess);
            // Note that stdout is buffered until '\n' is written, so if you
            // want to see the characters as they are computed, you should
            // call fflush(stdout); after printf().
            fflush(stdout);
        }
    }
    printf("\n");
    free(pages);
}
