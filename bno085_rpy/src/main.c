#define _POSIX_C_SOURCE 200809L
#include "bno085.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

static volatile sig_atomic_t running = 1;

static void stop(int signal_number)
{
    (void)signal_number;
    running = 0;
    bno085_request_stop();
}

static int reconnect_pause(void)
{
    struct timespec remaining = {
        RECONNECT_DELAY_MS / 1000,
        (RECONNECT_DELAY_MS % 1000) * 1000000L
    };
    while (running && nanosleep(&remaining, &remaining) < 0) {
        if (errno != EINTR) {
            perror("reconnect nanosleep");
            return -1;
        }
    }
    return 0;
}

int main(void)
{
    /* signal handler에서는 flag만 바꾸고 log/cleanup은 main에서 처리한다. */
    struct sigaction action = {0};
    action.sa_handler = stop;
    if (sigemptyset(&action.sa_mask) < 0 ||
        sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    while (running) {
        int result = bno085_init();
        if (result == 0 && running)
            result = bno085_enable_rotation_vector();
        while (result == 0 && running) {
            if (bno085_read() < 0 && running)
                result = -1;
        }
        int close_result = bno085_close();
        if (!running) {
            puts("[BNO085] Stop requested; resources closed");
            return close_result == 0 ? 0 : 1;
        }
        fprintf(stderr, "[WARN] BNO085 session failed; retry in %d ms (Ctrl+C to stop)\n",
                RECONNECT_DELAY_MS);
        if (reconnect_pause() < 0)
            return 1;
    }
    return 0;
}
