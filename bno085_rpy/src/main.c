#include "bno085.h"
#include <signal.h>

static volatile sig_atomic_t running = 1;

static void stop(int signal_number)
{
    (void)signal_number;
    running = 0;
}

int main(void)
{
    /* signal handler에서는 flag만 바꾸고 log/cleanup은 main에서 처리한다. */
    if (signal(SIGINT, stop) == SIG_ERR || signal(SIGTERM, stop) == SIG_ERR)
        return 1;
    int result = bno085_init();
    if (result == 0 && running)
        result = bno085_enable_rotation_vector();
    while (result == 0 && running) {
        if (bno085_read() < 0 && running)
            result = -1;
    }
    if (bno085_close() != 0)
        result = -1;
    return result == 0 ? 0 : 1;
}
