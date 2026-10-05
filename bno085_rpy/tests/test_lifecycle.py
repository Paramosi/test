"""Run production C with mocked GPIO/I2C/time; no sensor or real logs required.

Run: python3 tests/test_lifecycle.py (GCC required).
On Windows, temporary minimal Linux/POSIX declarations allow host-only tests.
These declarations are not a substitute for compiling on Raspberry Pi Linux.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

COMPAT = r'''
#include <stdint.h>
#include <signal.h>
#include <time.h>
typedef int clockid_t;
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define O_CLOEXEC 0
struct sigaction { void (*sa_handler)(int); int sa_mask; int sa_flags; };
static inline int sigemptyset(int *mask) { *mask = 0; return 0; }
struct gpio_v2_line_request {
    uint32_t offsets[64]; unsigned num_lines;
    struct { uint64_t flags; } config;
    char consumer[32]; int fd;
};
struct gpio_v2_line_values { uint64_t bits, mask; };
#define GPIO_V2_LINE_FLAG_INPUT 1
#define GPIO_V2_GET_LINE_IOCTL 1
#define GPIO_V2_LINE_GET_VALUES_IOCTL 2
#define I2C_FUNC_I2C 1
#define I2C_FUNCS 3
#define I2C_SLAVE 4
'''

DRIVER = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#ifdef _WIN32
#include "compat.h"
#endif
static double mock_time;
static unsigned cursor, events, writes;
static int gpio_high, cancel_sleep, read_error;
static uint8_t queue[32][32];
static size_t sizes[32];
static int fake_clock_gettime(clockid_t id, struct timespec *t) {
    (void)id; mock_time += 0.001; t->tv_sec = (time_t)mock_time;
    t->tv_nsec = (long)((mock_time - (double)t->tv_sec) * 1e9); return 0;
}
static int fake_nanosleep(const struct timespec *a, struct timespec *b);
static int fake_ioctl(int fd, unsigned long request, ...) {
    (void)fd; va_list ap; va_start(ap, request);
    if (request == GPIO_V2_LINE_GET_VALUES_IOCTL) {
        struct gpio_v2_line_values *v = va_arg(ap, struct gpio_v2_line_values *);
        v->bits = (uint64_t)gpio_high;
    }
    va_end(ap); return 0;
}
static ssize_t fake_read(int fd, void *data, size_t size) {
    (void)fd;
    if (read_error) { errno = EIO; return -1; }
    memset(data, 0, size);
    if (cursor < events) {
        assert(sizes[cursor] <= size);
        memcpy(data, queue[cursor], sizes[cursor]); ++cursor;
        if (cancel_sleep) gpio_high = 1;
    }
    return (ssize_t)size;
}
static ssize_t fake_write(int fd, const void *data, size_t size) {
    (void)fd; (void)data; ++writes; return (ssize_t)size;
}
static int fake_mkdir(const char *p, unsigned mode) { (void)p; (void)mode; return 0; }
#define clock_gettime fake_clock_gettime
#define nanosleep fake_nanosleep
#define ioctl fake_ioctl
#define read fake_read
#define write fake_write
#define mkdir fake_mkdir
#ifdef _WIN32
#undef isfinite
#define isfinite(x) __builtin_isfinite(x)
#endif
#include "DRIVER_SOURCE"
static int fake_nanosleep(const struct timespec *a, struct timespec *b) {
    (void)b; mock_time += (double)a->tv_sec + (double)a->tv_nsec / 1e9;
    if (cancel_sleep) { bno085_request_stop(); errno = EINTR; return -1; }
    return 0;
}
static void reset_test(void) {
    mock_time = 0; cursor = events = writes = 0;
    gpio_high = cancel_sleep = read_error = 0;
    stop_requested = 0; log_failed = false; feature_confirmed = false;
    memset(rx_seen, 0, sizeof(rx_seen)); memset(tx_sequence, 0, sizeof(tx_sequence));
    memset(queue, 0, sizeof(queue)); memset(sizes, 0, sizeof(sizes));
}
static void packet(unsigned channel, unsigned sequence, const uint8_t *p, size_t n) {
    assert(n + 4 <= 32 && events < 32);
    uint8_t *q = queue[events]; q[0] = (uint8_t)(n + 4);
    q[2] = (uint8_t)channel; q[3] = (uint8_t)sequence;
    memcpy(q + 4, p, n); sizes[events++] = n + 4;
}
static void empty(void) { sizes[events++] = 32; }
int main(void) {
    struct cargo c; packets = tmpfile(); assert(packets);
    const uint8_t advertisement[] = {0, 1}, reset[] = {1};
    uint8_t init[16] = {0xF1, 0, 0x84};
    reset_test(); empty(); packet(0, 0, advertisement, 2);
    packet(1, 0, reset, 1); packet(2, 0, init, 16);
    assert(wait_boot() == 0 && cursor == 4); /* reported failure scenario */
    reset_test(); assert(wait_boot() == -1 && mock_time >= 5 && mock_time < 5.1);
    reset_test(); gpio_high = 1;
    assert(wait_boot() == -1 && mock_time >= 5 && cursor == 0);
    reset_test(); read_error = 1;
    assert(wait_boot() == -1 && mock_time < 0.1); /* real I/O error stays an error */
    reset_test(); queue[0][0] = 0x14; queue[0][1] = 0x01;
    sizes[0] = 32; events = 1;
    for (unsigned i = 1; i < 10; ++i) {
        unsigned length = 276 - 28 * i;
        queue[i][0] = (uint8_t)length;
        queue[i][1] = (uint8_t)((length >> 8) | 0x80);
        queue[i][3] = (uint8_t)i;
        sizes[i] = i == 9 ? 24 : 32; ++events;
    }
    assert(receive_cargo(&c, deadline_after(500)) == 1);
    assert(c.payload_length == 272 && c.wire_bytes == 312 && cursor == 10);
    reset_test(); empty(); uint8_t product[16] = {0xF8};
    packet(2, 0, product, sizeof(product));
    assert(verify_product() == 0 && writes == 1 && cursor == 2);
    reset_test(); empty(); uint8_t feature[17] = {0xFC, 0x05};
    feature[5] = 0x40; feature[6] = 0x42; feature[7] = 0x0F;
    packet(2, 0, feature, sizeof(feature));
    assert(bno085_enable_rotation_vector() == 0 && writes == 2 && cursor == 2);
    reset_test(); assert(bno085_read() == 0 && mock_time >= 0.5 && writes == 0);
    reset_test(); cancel_sleep = 1;
    assert(wait_boot() == -1 && stop_requested && mock_time < 0.1);
    reset_test(); gpio_high = 1; cancel_sleep = 1;
    assert(wait_ready(deadline_after(5000)) == 0 && mock_time < 0.1);
    reset_test(); queue[0][0] = 40; sizes[0] = 32; events = 1; cancel_sleep = 1;
    assert(receive_cargo(&c, deadline_after(5000)) == 0 && mock_time < 0.1);
    reset_test(); packet(1, 0, reset, 1);
    assert(bno085_read() == -1); /* unexpected reset requests recovery */
    fclose(packets); puts("PASS: 12 driver lifecycle scenarios"); return 0;
}
'''

MAIN = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include "compat.h"
#endif
static int scenario, inits, enables, reads, closes, sleeps, requests;
static int fake_sigaction(int s, const struct sigaction *a, struct sigaction *b) {
    (void)s; (void)a; (void)b; return 0;
}
static int fake_nanosleep(const struct timespec *a, struct timespec *b);
#define sigaction(...) fake_sigaction(__VA_ARGS__)
#define nanosleep fake_nanosleep
#define main application_main
#include "MAIN_SOURCE"
#undef main
void bno085_request_stop(void) { ++requests; }
int bno085_init(void) {
    ++inits;
    if (scenario == 0 && inits == 1) return -1;
    if (scenario == 3) { stop(SIGINT); return -1; }
    if (scenario == 4) return -1;
    return 0;
}
int bno085_enable_rotation_vector(void) {
    ++enables; return scenario == 1 && enables == 1 ? -1 : 0;
}
int bno085_read(void) {
    ++reads;
    if (scenario == 2 && reads == 1) return -1;
    if (scenario == 5 && reads == 1) return 0; /* normal idle */
    stop(SIGINT); return 0;
}
int bno085_close(void) { ++closes; return scenario == 6 ? -1 : 0; }
static int fake_nanosleep(const struct timespec *a, struct timespec *b) {
    (void)b; assert(a->tv_sec == 1); ++sleeps;
    if (scenario == 4) { stop(SIGINT); errno = EINTR; return -1; }
    return 0;
}
int main(void) {
    for (scenario = 0; scenario < 7; ++scenario) {
        inits = enables = reads = closes = sleeps = requests = 0; running = 1;
        assert(application_main() == (scenario == 6 ? 1 : 0));
        assert(requests == 1);
        if (scenario < 3) assert(inits == 2 && closes == 2 && sleeps == 1);
        if (scenario == 3) assert(inits == 1 && enables == 0 && closes == 1 && sleeps == 0);
        if (scenario == 4) assert(inits == 1 && closes == 1 && sleeps == 1);
        if (scenario == 5) assert(inits == 1 && reads == 2 && closes == 1 && sleeps == 0);
        if (scenario == 6) assert(inits == 1 && closes == 1 && sleeps == 0);
    }
    puts("PASS: 7 supervisor lifecycle scenarios"); return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="bno085-tests-") as name:
        folder = Path(name)
        if os.name == "nt":
            (folder / "compat.h").write_text(COMPAT, encoding="utf-8")
            for header in ("linux/gpio.h", "linux/i2c.h", "linux/i2c-dev.h", "sys/ioctl.h"):
                path = folder / header
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("/* Host test placeholder; see compat.h. */\n", encoding="utf-8")
        for label, source in (("driver", DRIVER), ("supervisor", MAIN)):
            source = source.replace("DRIVER_SOURCE", (ROOT / "src/bno085.c").as_posix())
            source = source.replace("MAIN_SOURCE", (ROOT / "src/main.c").as_posix())
            path = folder / f"{label}.c"
            binary = folder / (label + (".exe" if os.name == "nt" else ""))
            path.write_text(source, encoding="utf-8")
            subprocess.run([os.environ.get("CC", "gcc"), "-std=c11", "-O2", "-Wall",
                            "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror",
                            "-DPRINT_RAW_PACKET=0", "-DPRINT_PROTOCOL=0",
                            "-I", str(folder), str(path), "-lm", "-o", str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True,
                                    encoding="utf-8", errors="replace", timeout=15, check=False)
            if result.returncode:
                print(result.stdout[-6000:])
                print(result.stderr[-6000:])
                raise SystemExit(f"{label} failed ({result.returncode})")
            print(result.stdout.splitlines()[-1])


if __name__ == "__main__":
    main()
