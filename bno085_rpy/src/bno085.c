#define _POSIX_C_SOURCE 200809L
#include "bno085.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* 출처의 URL, Revision, Page 및 GitHub Commit은 docs/REFERENCES.md에 있다.
 * [BNO] = BNO08X Datasheet, [SHTP] = Sensor Hub Transport Protocol,
 * [SH2] = SH-2 Reference Manual, [LINUX] = Linux Kernel documentation. */
static int i2c_fd = -1;
static int int_fd = -1;
static FILE *communication;
static FILE *packets;
static FILE *rpy_csv;
static bool log_failed;
/* [SHTP] §2.2.1: channel과 direction마다 독립된 sequence를 사용한다. */
static uint8_t tx_sequence[SHTP_CHANNEL_COUNT];
static uint8_t rx_next[SHTP_CHANNEL_COUNT];
static bool rx_seen[SHTP_CHANNEL_COUNT];
static bool feature_confirmed;
static bool sensor_seen;
static uint8_t sensor_next;
static double last_report_host_time;

struct cargo {
    uint16_t length;                 /* 첫 transfer의 header + cargo length */
    uint8_t channel, first_sequence, last_sequence;
    size_t payload_length, wire_bytes;
    uint8_t payload[RX_CARGO_CAPACITY];
};

static double seconds(clockid_t clock_id)
{
    struct timespec t;
    if (clock_gettime(clock_id, &t) != 0) {
        perror("clock_gettime");
        return -1.0;
    }
    return (double)t.tv_sec + (double)t.tv_nsec / 1000000000.0;
}

static double deadline_after(int milliseconds)
{
    double now = seconds(CLOCK_MONOTONIC);
    return now < 0.0 ? -1.0 : now + (double)milliseconds / 1000.0;
}

static void message(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    if (communication) {
        va_start(args, format);
        vfprintf(communication, format, args);
        va_end(args);
        if (fflush(communication) != 0 || ferror(communication))
            log_failed = true;
    }
    fflush(stdout);
}

static int system_error(const char *operation)
{
    int saved_errno = errno;
    message("[ERROR] %s: errno=%d (%s)\n", operation,
            saved_errno, strerror(saved_errno));
    return -1;
}

/* 성공한 syscall의 실제 byte를 기록한다. TX 실패의 wire 상태는 알 수 없다.
 * 이 log는 I2C address/ACK/SCL의 logic analyzer capture가 아니다. */
static void log_transfer(const char *direction, const uint8_t *data, size_t count)
{
    double host_time = seconds(CLOCK_REALTIME);
    fprintf(packets, "TIME: %.6f (Unix seconds)\nDIRECTION: %s\nLENGTH: %zu\nHEX:\n",
            host_time, direction, count);
    for (size_t i = 0; i < count; ++i)
        fprintf(packets, "%02X%c", (unsigned)data[i],
                (i + 1 == count || (i + 1) % 16 == 0) ? '\n' : ' ');
    fputc('\n', packets);
    if (fflush(packets) != 0 || ferror(packets) || host_time < 0.0)
        log_failed = true;
    if (PRINT_RAW_PACKET) {
        message("[SHTP %s] I2C %zu bytes; HEX:", direction, count);
        for (size_t i = 0; i < count; ++i)
            message(" %02X", (unsigned)data[i]);
        message("\n");
    }
    if (PRINT_BITS) {
        for (size_t i = 0; i < count; ++i) {
            message("  byte[%zu] = ", i);
            for (int bit = 7; bit >= 0; --bit)
                message("%u", (unsigned)((data[i] >> bit) & 1u));
            message("\n");
        }
    }
}

/* [SH2] §6.3.2: multi-byte field는 little-endian이다.
 * integer pointer로 cast하지 않아 alignment/endianness 의존을 피한다. */
static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* two's complement raw 값의 부호를 명시적으로 복원한다.
 * 범위 밖 unsigned -> signed cast의 implementation-defined 결과를 피한다.
 * [SH2] §6.5.18.2 field와 CEVA sh2_SensorValue.c::decodeRotationVector
 * 교차검증. Q-format scaling은 raw decoding 다음에 수행한다. */
static int16_t signed_le16(const uint8_t *p)
{
    uint16_t value = le16(p);
    int32_t signed_value = value <= INT16_MAX ? (int32_t)value : (int32_t)value - 65536;
    return (int16_t)signed_value;
}

static int32_t signed_le32(const uint8_t *p)
{
    uint32_t value = le32(p);
    int64_t signed_value = value <= INT32_MAX ? (int64_t)value : (int64_t)value - INT64_C(4294967296);
    return (int32_t)signed_value;
}

/* [LINUX GPIO] GPIO v2 API: INPUT을 요청한다. ACTIVE_LOW flag를 설정하지
 * 않으므로 bits=0이 physical LOW이며 [BNO] H_INTN asserted 상태이다. */
static int open_interrupt_line(void)
{
    struct gpio_v2_line_request request = {0};
    int chip_fd = open(GPIO_CHIP, O_RDONLY | O_CLOEXEC);
    if (chip_fd < 0)
        return system_error("open GPIO_CHIP");
    request.offsets[0] = H_INTN_LINE;
    request.num_lines = 1;
    request.config.flags = GPIO_V2_LINE_FLAG_INPUT;
    snprintf(request.consumer, sizeof(request.consumer), "bno085-rpy");
    if (ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &request) < 0) {
        system_error("GPIO_V2_GET_LINE_IOCTL");
        close(chip_fd);
        return -1;
    }
    int_fd = request.fd;
    close(chip_fd);
    message("[GPIO] %s line %u: H_INTN INPUT, active LOW ... OK\n",
            GPIO_CHIP, (unsigned)H_INTN_LINE);
    return 0;
}

static int data_ready(void)
{
    struct gpio_v2_line_values values = {0};
    values.mask = 1;
    if (ioctl(int_fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values) < 0)
        return system_error("GPIO_V2_LINE_GET_VALUES_IOCTL");
    return (values.bits & 1u) == 0 ? 1 : 0;
}

/* [BNO] §1.2.2.2 p.15: empty read는 clock stretching으로 대기할 수 있다.
 * [SHTP] §3.4.1 Figure 4: H_INTN을 확인한 후에만 read한다.
 * deadline은 GPIO 대기를 제한한다. read/write syscall 내부의 controller
 * timeout까지 이 userspace deadline으로 강제 중단할 수는 없다. */
static int wait_ready(double deadline)
{
    const struct timespec pause = {0, 1000000}; /* host polling: 1 ms */
    if (deadline < 0.0)
        return -1;
    for (;;) {
        double now = seconds(CLOCK_MONOTONIC);
        if (now < 0.0)
            return -1;
        if (now >= deadline)
            return 0;
        int ready = data_ready();
        if (ready != 0)
            return ready;
        if (nanosleep(&pause, NULL) < 0 && errno != EINTR)
            return system_error("nanosleep");
    }
}

/* [LINUX I2C] C example: I2C_SLAVE 선택 후 plain write를 사용한다.
 * [SHTP] §3.2: transfer마다 STOP이 필요하다. register address를 보내지
 * 않으며 repeated START를 만드는 combined I2C_RDWR도 사용하지 않는다.
 * reset/Product ID/Feature command는 작으므로 fragmented write는 없다. */
static int send_packet(uint8_t channel, const uint8_t *payload, size_t size)
{
    uint8_t wire[I2C_RX_TRANSFER_BYTES];
    size_t length = size + SHTP_HEADER_BYTES;
    if (channel >= SHTP_CHANNEL_COUNT || size == 0 || length > sizeof(wire))
        return -1;
    wire[0] = (uint8_t)(length & 0xFFu);
    wire[1] = (uint8_t)(length >> 8);
    wire[2] = channel;
    wire[3] = tx_sequence[channel];
    memcpy(wire + SHTP_HEADER_BYTES, payload, size);
    ssize_t count = write(i2c_fd, wire, length);
    if (count > 0)
        log_transfer("TX", wire, (size_t)count);
    if (count < 0)
        return system_error("I2C write (wire result unknown)");
    if ((size_t)count != length) {
        message("[ERROR] short I2C write: %zd/%zu; remainder 재전송 안 함\n",
                count, length);
        return -1;
    }
    ++tx_sequence[channel];
    return log_failed ? -1 : 0;
}

/* [SHTP] §§2.2.1, 2.3.1, 3.4.1: 길이를 예측하여 최대 32 byte를 읽는다.
 * 짧은 cargo 뒤의 zero padding은 protocol에서 허용한다. 긴 cargo는
 * 다음 transfer의 header를 다시 해석하고 remaining payload만 조립한다.
 * continuation의 channel/sequence/remaining length를 모두 검사한다. */
static int receive_cargo(struct cargo *out, double deadline)
{
    uint8_t wire[I2C_RX_TRANSFER_BYTES];
    size_t filled = 0, remaining = 0;
    bool first = true;
    memset(out, 0, sizeof(*out));
    for (;;) {
        int ready = wait_ready(deadline);
        if (ready <= 0) {
            if (!first)
                message("[ERROR] incomplete cargo: continuation timeout/error\n");
            return first ? ready : -1;
        }
        size_t requested = first ? sizeof(wire) : remaining + SHTP_HEADER_BYTES;
        if (requested > sizeof(wire))
            requested = sizeof(wire);
        ssize_t count = read(i2c_fd, wire, requested);
        if (count > 0)
            log_transfer("RX", wire, (size_t)count);
        if (count < 0)
            return system_error("I2C read");
        if ((size_t)count != requested) {
            message("[ERROR] short I2C read: %zd/%zu; cargo 폐기\n", count, requested);
            return -1;
        }
        if (log_failed)
            return -1;
        uint16_t encoded = le16(wire);
        uint16_t length = (uint16_t)(encoded & SHTP_LENGTH_MASK);
        uint8_t channel = wire[2], sequence = wire[3];
        bool continuation = (encoded & SHTP_CONTINUATION) != 0;
        out->wire_bytes += (size_t)count;
        if (PRINT_PROTOCOL)
            message("[SHTP] length=%u channel=%u sequence=%u continuation=%u\n",
                    (unsigned)length, (unsigned)channel, (unsigned)sequence,
                    (unsigned)continuation);
        if (encoded == 0 && first)
            return 0; /* null header: 성공한 boot packet으로 세지 않는다. */
        if (encoded == SHTP_ERROR_LENGTH || length <= SHTP_HEADER_BYTES ||
            length > SHTP_MAX_LENGTH || channel >= SHTP_CHANNEL_COUNT) {
            message("[ERROR] invalid SHTP header: encoded length=0x%04X\n",
                    (unsigned)encoded);
            return -1;
        }
        size_t available = (size_t)length - SHTP_HEADER_BYTES;
        if (first) {
            if (continuation || available > sizeof(out->payload)) {
                message("[ERROR] orphan continuation or host buffer overflow\n");
                return -1;
            }
            out->length = length;
            out->channel = channel;
            out->first_sequence = sequence;
            out->payload_length = available;
            remaining = available;
        } else if (!continuation || channel != out->channel ||
                   sequence != (uint8_t)(out->last_sequence + 1u) ||
                   available != remaining) {
            message("[ERROR] continuation channel/sequence/remaining length mismatch\n");
            return -1;
        }
        if (rx_seen[channel] && sequence != rx_next[channel])
            message("[WARN] RX channel %u sequence: expected=%u received=%u\n",
                    (unsigned)channel, (unsigned)rx_next[channel], (unsigned)sequence);
        rx_seen[channel] = true;
        rx_next[channel] = (uint8_t)(sequence + 1u);
        out->last_sequence = sequence;
        size_t copied = (size_t)count - SHTP_HEADER_BYTES;
        if (copied > remaining)
            copied = remaining; /* padding을 cargo에 넣지 않는다. */
        memcpy(out->payload + filled, wire + SHTP_HEADER_BYTES, copied);
        filled += copied;
        remaining -= copied;
        if (remaining == 0) {
            if (PRINT_PROTOCOL)
                message("[SHTP CARGO] payload=%zu bytes; I2C RX total=%zu bytes\n",
                        out->payload_length, out->wire_bytes);
            return 1;
        }
        first = false;
    }
}

/* reset 이전의 packet을 새 boot evidence로 잘못 세지 않는다.
 * [SHTP] §2.3.1: 초기 advertisement가 완료되기 전에 write하지 않는다. */
static int drain_pending(void)
{
    struct cargo old;
    double deadline = deadline_after(BOOT_TIMEOUT_MS);
    if (deadline < 0.0)
        return -1;
    message("[BNO085] reset 이전 pending cargo drain\n");
    for (;;) {
        double now = seconds(CLOCK_MONOTONIC);
        if (now < 0.0)
            return -1;
        if (now >= deadline) {
            message("[ERROR] pending cargo drain timeout\n");
            return -1;
        }
        double slice = now + (double)READ_TIMEOUT_MS / 1000.0;
        if (slice > deadline)
            slice = deadline;
        int result = receive_cargo(&old, slice);
        if (result <= 0)
            return result;
        message("[BNO085] pre-reset cargo 기록 완료; boot evidence에서는 제외\n");
    }
}

/* [BNO] §5.2.1 p.43: advertisement, executable reset complete,
 * SH-2 unsolicited initialization을 각각 확인한다. [SH2] §6.4.4.2:
 * payload offset 2=0x84, offset 5=Status, 0=successful. */
static int wait_boot(void)
{
    struct cargo c;
    bool advertisement = false, reset = false, initialize = false;
    double deadline = deadline_after(BOOT_TIMEOUT_MS);
    for (;;) {
        int result = receive_cargo(&c, deadline);
        if (result <= 0) {
            message("[ERROR] boot incomplete: advertisement=%u reset=%u initialize=%u\n",
                    (unsigned)advertisement, (unsigned)reset, (unsigned)initialize);
            return -1;
        }
        if (c.channel == SHTP_CH_COMMAND && c.payload_length > 1 &&
            c.payload[0] == SHTP_ADVERTISEMENT) {
            advertisement = true;
            message("[BNO085] SHTP advertisement received\n");
        } else if (c.channel == SHTP_CH_EXECUTABLE && c.payload_length == 1 &&
                   c.payload[0] == EXECUTABLE_RESET) {
            reset = true;
            message("[BNO085] executable reset complete received\n");
        } else if (c.channel == SHTP_CH_CONTROL) {
            /* 알려진 fixed-length Command Response만 검사한다. */
            size_t offset = 0;
            while (offset + SH2_COMMAND_RESPONSE_BYTES <= c.payload_length &&
                   c.payload[offset] == SH2_COMMAND_RESPONSE) {
                if (c.payload[offset + 2] == SH2_INITIALIZE_UNSOLICITED) {
                    if (c.payload[offset + 5] != 0) {
                        message("[ERROR] SH-2 initialization Status=%u\n",
                                (unsigned)c.payload[offset + 5]);
                        return -1;
                    }
                    initialize = true;
                    message("[BNO085] SH-2 unsolicited initialization Status=0\n");
                }
                offset += SH2_COMMAND_RESPONSE_BYTES;
            }
        }
        if (advertisement && reset && initialize)
            return 0;
    }
}

/* [SH2] §§6.3.1–2, Figures 33–34: F9 00 -> F8, 16-byte response.
 * Status나 sensor report가 아니라 firmware 식별 정보이다. */
static int verify_product(void)
{
    const uint8_t request[] = {SH2_PRODUCT_ID_REQUEST, 0};
    struct cargo c;
    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) {
        message("[SH-2] Product ID Request (%d/%d)\n", attempt, MAX_RETRIES);
        if (send_packet(SHTP_CH_CONTROL, request, sizeof(request)) < 0)
            return -1; /* TX 오류는 자동 재전송하지 않는다. */
        double deadline = deadline_after(READ_TIMEOUT_MS);
        for (;;) {
            int result = receive_cargo(&c, deadline);
            if (result < 0)
                return -1;
            if (result == 0)
                break;
            if (c.channel != SHTP_CH_CONTROL)
                continue;
            size_t offset = 0;
            while (offset < c.payload_length) {
                const uint8_t *p = c.payload + offset;
                size_t left = c.payload_length - offset;
                if (p[0] == SH2_PRODUCT_ID_RESPONSE) {
                    if (left < SH2_PRODUCT_ID_BYTES) {
                        message("[ERROR] truncated Product ID Response\n");
                        return -1;
                    }
                    message("[SH-2] Product ID Response: reset_cause=%u\n"
                            "  SW version=%u.%u.%u part=%" PRIu32 " build=%" PRIu32 "\n",
                            (unsigned)p[1], (unsigned)p[2], (unsigned)p[3],
                            (unsigned)le16(p + 12), le32(p + 4), le32(p + 8));
                    return 0;
                }
                if (p[0] != SH2_COMMAND_RESPONSE || left < SH2_COMMAND_RESPONSE_BYTES) {
                    message("[SH-2] control report 0x%02X: raw log에만 기록\n",
                            (unsigned)p[0]);
                    break; /* 알 수 없는 report의 length를 추측하지 않는다. */
                }
                offset += SH2_COMMAND_RESPONSE_BYTES;
            }
        }
        message("[WARN] Product ID Response timeout; idempotent query만 retry\n");
    }
    return -1;
}

struct rotation_raw {
    uint8_t sequence, status;
    uint16_t delay_ticks;
    int16_t i, j, k, real, accuracy;
};

/* [SH2] §§6.5.1, 6.5.18.2: report 시작 기준 offset이다.
 * SHTP header나 Base Timestamp 5 byte를 이 offset에 다시 더하지 않는다. */
static int decode_rotation_report(const uint8_t *p, size_t length, struct rotation_raw *r)
{
    if (length < SH2_ROTATION_VECTOR_BYTES || p[0] != SH2_ROTATION_VECTOR)
        return -1;
    r->sequence = p[SH2_SENSOR_SEQUENCE_OFFSET];
    r->status = (uint8_t)(p[SH2_SENSOR_STATUS_OFFSET] & SH2_STATUS_MASK);
    r->delay_ticks = (uint16_t)(((uint16_t)(p[SH2_SENSOR_STATUS_OFFSET] & SH2_DELAY_UPPER_MASK) << 6) |
                              p[SH2_SENSOR_DELAY_OFFSET]);
    r->i = signed_le16(p + SH2_RV_I_OFFSET);
    r->j = signed_le16(p + SH2_RV_J_OFFSET);
    r->k = signed_le16(p + SH2_RV_K_OFFSET);
    r->real = signed_le16(p + SH2_RV_REAL_OFFSET);
    r->accuracy = signed_le16(p + SH2_RV_ACCURACY_OFFSET);
    return 0;
}

static void print_raw_field(const char *name, const uint8_t *p, int16_t value)
{
    message("  %s\n"
            "    raw bytes (LSB MSB): %02X %02X\n"
            "    little-endian: (0x%02X << 8) | 0x%02X = 0x%04X\n"
            "    unsigned=%u; signed=%d\n",
            name, (unsigned)p[0], (unsigned)p[1], (unsigned)p[1], (unsigned)p[0],
            (unsigned)le16(p), (unsigned)le16(p), (int)value);
}

struct orientation {
    double x, y, z, w, norm, accuracy_rad;
    double roll_deg, pitch_deg, yaw_deg;
    bool pitch_clamped;
};

/* [SH2] §6.5.18 p.71: i/j/k/real -> x/y/z/w, Quaternion Q14,
 * accuracy Q12 radians. norm은 validation 출력용이며 나누지 않는다.
 * [MATH] NASA-TM-74839 §2.2 Eq.(15), Appendix A ZYX (3,2,1), p.A-11:
 * scalar-first q1/q2/q3/q4 = w/x/y/z. R = Rz(yaw) Ry(pitch) Rx(roll).
 * unit Quaternion 조건에서 matrix element를 대입한 표준 ZYX 식이다.
 * CEVA 문서가 제공한 Euler 식이라고 주장하지 않는다. */
static int rotation_to_euler(const struct rotation_raw *raw, struct orientation *o)
{
    const double quaternion_scale = (double)(1u << SH2_RV_QUATERNION_Q);
    const double accuracy_scale = (double)(1u << SH2_RV_ACCURACY_Q);
    const double rad_to_deg = 180.0 / acos(-1.0);
    o->x = (double)raw->i / quaternion_scale;
    o->y = (double)raw->j / quaternion_scale;
    o->z = (double)raw->k / quaternion_scale;
    o->w = (double)raw->real / quaternion_scale;
    o->accuracy_rad = (double)raw->accuracy / accuracy_scale;
    o->norm = sqrt(o->x * o->x + o->y * o->y + o->z * o->z + o->w * o->w);
    if (!isfinite(o->norm) || o->norm == 0.0)
        return -1; /* zero Quaternion은 orientation을 표현하지 못한다. */
    double sin_pitch = 2.0 * (o->w * o->y - o->z * o->x);
    o->pitch_clamped = sin_pitch < -1.0 || sin_pitch > 1.0;
    /* Q14 quantization으로 asin domain을 벗어날 수 있다.
     * scalar argument만 제한하며 Quaternion normalization은 하지 않는다. */
    if (sin_pitch > 1.0)
        sin_pitch = 1.0;
    else if (sin_pitch < -1.0)
        sin_pitch = -1.0;
    o->roll_deg = atan2(2.0 * (o->w * o->x + o->y * o->z),
                       1.0 - 2.0 * (o->x * o->x + o->y * o->y)) * rad_to_deg;
    o->pitch_deg = asin(sin_pitch) * rad_to_deg;
    o->yaw_deg = atan2(2.0 * (o->w * o->z + o->x * o->y),
                      1.0 - 2.0 * (o->y * o->y + o->z * o->z)) * rad_to_deg;
    return isfinite(o->roll_deg) && isfinite(o->pitch_deg) && isfinite(o->yaw_deg) ? 0 : -1;
}

/* append mode에서 header를 한 번만 쓴다. 기존 schema가 다르거나 마지막
 * record가 incomplete하면 덧붙이지 않고 오류로 종료한다. */
static int open_rpy_csv(void)
{
    static const char header[] =
        "host_time_s,shtp_sequence,sensor_sequence,roll_deg,pitch_deg,yaw_deg,status";
    char existing[128];
    rpy_csv = fopen("logs/rpy.csv", "a+");
    if (!rpy_csv)
        return system_error("fopen rpy.csv");
    if (fseek(rpy_csv, 0, SEEK_END) != 0)
        return system_error("seek rpy.csv");
    long size = ftell(rpy_csv);
    if (size < 0)
        return system_error("size rpy.csv");
    if (size == 0) {
        if (fprintf(rpy_csv, "%s\n", header) < 0 || fflush(rpy_csv) != 0)
            return system_error("write rpy.csv header");
    } else {
        rewind(rpy_csv);
        if (!fgets(existing, sizeof(existing), rpy_csv))
            return system_error("read rpy.csv header");
        existing[strcspn(existing, "\r\n")] = '\0';
        if (strcmp(existing, header) != 0) {
            message("[ERROR] rpy.csv schema mismatch; 기존 file 보존\n");
            return -1;
        }
        if (fseek(rpy_csv, -1, SEEK_END) != 0)
            return system_error("seek rpy.csv tail");
        if (fgetc(rpy_csv) != '\n') {
            message("[ERROR] rpy.csv incomplete final record; 기존 file 보존\n");
            return -1;
        }
        if (fseek(rpy_csv, 0, SEEK_END) != 0)
            return system_error("seek rpy.csv append");
    }
    return 0;
}

static int log_orientation(const struct cargo *c, const struct rotation_raw *raw,
                           const struct orientation *o, double receipt_time)
{
    message("[QUATERNION]\n"
            "  Q14: x=i/16384=%.9f y=j/16384=%.9f\n"
            "       z=k/16384=%.9f w=real/16384=%.9f\n"
            "  norm=%.9f (validation only; normalization 없음)\n"
            "  heading accuracy Q12=%.9f rad\n",
            o->x, o->y, o->z, o->w, o->norm, o->accuracy_rad);
    if (o->pitch_clamped)
        message("[WARN] asin argument clamped to [-1,1]; raw Quaternion/norm 확인 필요\n");
    message("[EULER ZYX]\n  Roll  = %.6f deg\n  Pitch = %.6f deg\n  Yaw   = %.6f deg\n",
            o->roll_deg, o->pitch_deg, o->yaw_deg);
    if (log_failed)
        return -1;
    /* host_time_s는 report 처리 시 CLOCK_REALTIME; sensor timestamp가 아니다.
     * Status=0도 기록하여 quality 판단을 사용자가 할 수 있게 한다. */
    if (fprintf(rpy_csv, "%.6f,%u,%u,%.9f,%.9f,%.9f,%u\n", receipt_time,
                (unsigned)c->first_sequence, (unsigned)raw->sequence,
                o->roll_deg, o->pitch_deg, o->yaw_deg, (unsigned)raw->status) < 0 ||
        fflush(rpy_csv) != 0 || ferror(rpy_csv)) {
        log_failed = true;
        return system_error("write rpy.csv");
    }
    return 0;
}

/* [SH2] §7.2: 하나의 cargo에서 여러 report를 fixed length로 분리한다.
 * 다른 sensor의 report length를 추측하거나 payload에서 0x05를 검색하지 않는다.
 * Base/Rebase는 상대시간만 해석한다. GPIO polling 시각은 H_INTN edge의
 * 정확한 시각이 아니므로 absolute sensor timestamp로 변환하지 않는다. */
static int parse_sensor_cargo(const struct cargo *c)
{
    static const char *const status_names[] = {
        "Unreliable", "Accuracy low", "Accuracy medium", "Accuracy high"
    };
    size_t offset = 0;
    bool base_seen = false, time_valid = false;
    int64_t reference_ticks = 0;
    int count = 0;
    while (offset < c->payload_length) {
        const uint8_t *p = c->payload + offset;
        size_t left = c->payload_length - offset;
        if (p[0] == SH2_BASE_TIMESTAMP || p[0] == SH2_TIMESTAMP_REBASE) {
            if (left < SH2_TIMESTAMP_BYTES) {
                message("[ERROR] truncated Base Timestamp/Rebase\n");
                return -1;
            }
            int32_t delta = signed_le32(p + SH2_TIMESTAMP_DATA_OFFSET);
            if (p[0] == SH2_BASE_TIMESTAMP) {
                base_seen = true;
                time_valid = le32(p + SH2_TIMESTAMP_DATA_OFFSET) != SH2_BASE_DELTA_UNAVAILABLE;
                reference_ticks = -(int64_t)delta;
                message("[SH-2] Base Timestamp: raw=0x%08" PRIX32 " delta=%" PRId32
                        " ticks (100 us/tick)%s\n", le32(p + 1), delta,
                        time_valid ? "" : " [reserved: unavailable]");
            } else {
                if (!base_seen) {
                    message("[ERROR] Timestamp Rebase without Base Timestamp\n");
                    return -1;
                }
                reference_ticks += delta;
                message("[SH-2] Timestamp Rebase: delta=%" PRId32 " ticks (100 us/tick)\n", delta);
            }
            offset += SH2_TIMESTAMP_BYTES;
            continue;
        }
        if (p[0] != SH2_ROTATION_VECTOR) {
            message("[WARN] unexpected sensor report 0x%02X at payload offset %zu;"
                    " 남은 cargo는 raw log에만 기록\n", (unsigned)p[0], offset);
            break;
        }
        struct rotation_raw r;
        if (!base_seen || decode_rotation_report(p, left, &r) < 0) {
            message("[ERROR] Rotation Vector: missing Base Timestamp or truncated 14-byte report\n");
            return -1;
        }
        if (sensor_seen && r.sequence != sensor_next)
            message("[WARN] sensor sequence gap/duplicate: expected=%u received=%u\n",
                    (unsigned)sensor_next, (unsigned)r.sequence);
        double host_time = seconds(CLOCK_MONOTONIC);
        double receipt_time = seconds(CLOCK_REALTIME);
        if (host_time < 0.0 || receipt_time < 0.0)
            return -1;
        message("\n[ROTATION VECTOR RAW REPORT]\n"
                "I2C RX total=%zu bytes\n"
                "[SHTP]\n  length=%u channel=%u sequence=%u last_sequence=%u\n"
                "[SH-2]\n  report=Rotation Vector (0x05)\n"
                "  sensor sequence=%u\n  Status byte=0x%02X; status=%u (%s)\n"
                "  Delay=((0x%02X & 0xFC) << 6) | 0x%02X = %u ticks = %u us\n",
                c->wire_bytes, (unsigned)c->length, (unsigned)c->channel,
                (unsigned)c->first_sequence, (unsigned)c->last_sequence,
                (unsigned)r.sequence, (unsigned)p[SH2_SENSOR_STATUS_OFFSET], (unsigned)r.status,
                status_names[r.status], (unsigned)p[SH2_SENSOR_STATUS_OFFSET],
                (unsigned)p[SH2_SENSOR_DELAY_OFFSET], (unsigned)r.delay_ticks,
                (unsigned)r.delay_ticks * SH2_TIME_TICK_US);
        if (time_valid)
            message("  relative to transport H_INTN reference=%" PRId64 " us (absolute time 계산 안 함)\n",
                    (reference_ticks + r.delay_ticks) * SH2_TIME_TICK_US);
        else
            message("  transport reference offset unavailable\n");
        if (sensor_seen)
            message("  host receipt interval=%.6f s (arrival timing)\n", host_time - last_report_host_time);
        message("[ROTATION VECTOR RAW]\n");
        print_raw_field("i", p + SH2_RV_I_OFFSET, r.i);
        print_raw_field("j", p + SH2_RV_J_OFFSET, r.j);
        print_raw_field("k", p + SH2_RV_K_OFFSET, r.k);
        print_raw_field("real", p + SH2_RV_REAL_OFFSET, r.real);
        print_raw_field("accuracy", p + SH2_RV_ACCURACY_OFFSET, r.accuracy);
        struct orientation o;
        if (rotation_to_euler(&r, &o) < 0) {
            message("[WARN] invalid Quaternion norm=%.9f; RPY/CSV row 생성 안 함\n", o.norm);
        } else {
            if (log_orientation(c, &r, &o, receipt_time) < 0)
                return -1;
            message("[STAGE 3] Rotation Vector -> Quaternion -> RPY logged\n");
        }
        if (!sensor_seen)
            message("[STAGE 2] complete Rotation Vector raw report received\n");
        sensor_seen = true;
        sensor_next = (uint8_t)(r.sequence + 1u);
        last_report_host_time = host_time;
        ++count;
        offset += SH2_ROTATION_VECTOR_BYTES;
    }
    return log_failed ? -1 : count;
}

/* [SH2] §§6.5.3–5: 0xFC는 Get Feature Response이다.
 * command response와 혼동하지 않고 실제 설정값을 검사한다. */
static int parse_control_cargo(const struct cargo *c)
{
    size_t offset = 0;
    while (offset < c->payload_length) {
        const uint8_t *p = c->payload + offset;
        size_t left = c->payload_length - offset;
        size_t length;
        if (p[0] == SH2_GET_FEATURE_RESPONSE)
            length = SH2_FEATURE_BYTES;
        else if (p[0] == SH2_COMMAND_RESPONSE)
            length = SH2_COMMAND_RESPONSE_BYTES;
        else if (p[0] == SH2_PRODUCT_ID_RESPONSE)
            length = SH2_PRODUCT_ID_BYTES;
        else {
            message("[SH-2] control report 0x%02X: raw log에만 기록\n", (unsigned)p[0]);
            break;
        }
        if (left < length) {
            message("[ERROR] truncated control report 0x%02X\n", (unsigned)p[0]);
            return -1;
        }
        if (p[0] == SH2_COMMAND_RESPONSE && p[2] == SH2_INITIALIZE_UNSOLICITED) {
            message("[ERROR] unexpected SH-2 reset; stream 종료 후 다시 초기화 필요\n");
            return -1;
        }
        if (p[0] == SH2_GET_FEATURE_RESPONSE) {
            uint32_t interval = le32(p + SH2_FEATURE_INTERVAL_OFFSET);
            message("[SH-2] Get Feature Response: feature=0x%02X flags=0x%02X interval=%" PRIu32
                    " us batch=%" PRIu32 " us\n", (unsigned)p[SH2_FEATURE_ID_OFFSET],
                    (unsigned)p[SH2_FEATURE_FLAGS_OFFSET], interval, le32(p + SH2_FEATURE_BATCH_OFFSET));
            if (p[SH2_FEATURE_ID_OFFSET] == SH2_ROTATION_VECTOR) {
                if (interval != REPORT_INTERVAL_US || p[SH2_FEATURE_FLAGS_OFFSET] != 0 ||
                    le16(p + SH2_FEATURE_CHANGE_OFFSET) != 0 || le32(p + SH2_FEATURE_BATCH_OFFSET) != 0 ||
                    le32(p + SH2_FEATURE_CONFIG_OFFSET) != 0) {
                    message("[ERROR] Rotation Vector feature differs from requested 1 Hz configuration\n");
                    return -1;
                }
                feature_confirmed = true;
            }
        }
        offset += length;
    }
    return log_failed ? -1 : 0;
}

static int process_stream_cargo(const struct cargo *c)
{
    if (c->channel == SHTP_CH_REPORTS)
        return parse_sensor_cargo(c);
    if (c->channel == SHTP_CH_CONTROL)
        return parse_control_cargo(c);
    if (c->channel == SHTP_CH_EXECUTABLE && c->payload_length == 1 && c->payload[0] == EXECUTABLE_RESET) {
        message("[ERROR] unexpected executable reset complete; stream 종료\n");
        return -1;
    }
    message("[SHTP] channel %u: raw log에만 기록\n", (unsigned)c->channel);
    return 0;
}

int bno085_enable_rotation_vector(void)
{
    uint8_t feature[SH2_FEATURE_BYTES] = {0};
    const uint8_t get_feature[] = {SH2_GET_FEATURE_REQUEST, SH2_ROTATION_VECTOR};
    uint32_t interval = REPORT_INTERVAL_US;
    feature[0] = SH2_SET_FEATURE_COMMAND;
    feature[SH2_FEATURE_ID_OFFSET] = SH2_ROTATION_VECTOR;
    /* [SH2] Figure 78: Report Interval는 payload offset 5–8의 little-endian. */
    for (unsigned byte = 0; byte < 4; ++byte)
        feature[SH2_FEATURE_INTERVAL_OFFSET + byte] = (uint8_t)(interval >> (byte * 8u));
    feature_confirmed = false;
    message("[SH-2] Set Feature: Rotation Vector only, interval=%" PRIu32 " us (1 Hz)\n", interval);
    if (send_packet(SHTP_CH_CONTROL, feature, sizeof(feature)) < 0 ||
        send_packet(SHTP_CH_CONTROL, get_feature, sizeof(get_feature)) < 0)
        return -1;
    struct cargo c;
    double deadline = deadline_after(BOOT_TIMEOUT_MS);
    while (!feature_confirmed) {
        int result = receive_cargo(&c, deadline);
        if (result <= 0) {
            message("[ERROR] matching Rotation Vector Get Feature Response timeout/error\n");
            return -1;
        }
        if (process_stream_cargo(&c) < 0)
            return -1;
    }
    message("[SH-2] Rotation Vector configuration confirmed: 1000000 us\n");
    return 0;
}

int bno085_read(void)
{
    struct cargo c;
    int result = receive_cargo(&c, deadline_after(READ_TIMEOUT_MS));
    /* 1 Hz에서 500 ms 무수신은 정상이다. report를 새로 요청하지 않는다. */
    return result <= 0 ? result : process_stream_cargo(&c);
}

int bno085_init(void)
{
    unsigned long functions = 0;
    const uint8_t reset[] = {EXECUTABLE_RESET};
    log_failed = false;
    memset(tx_sequence, 0, sizeof(tx_sequence));
    memset(rx_seen, 0, sizeof(rx_seen));
    feature_confirmed = false;
    sensor_seen = false;
    if (mkdir("logs", 0755) != 0 && errno != EEXIST)
        return system_error("mkdir logs");
    communication = fopen("logs/communication.log", "a");
    if (!communication)
        return system_error("fopen communication.log");
    packets = fopen("logs/packets.log", "a");
    if (!packets)
        return system_error("fopen packets.log");
    if (open_rpy_csv() < 0)
        return -1;
    message("================================================\n"
            "BNO085 SESSION START — Stage 3\n"
            "host_time_s=%.6f (Unix seconds)\n"
            "================================================\n", seconds(CLOCK_REALTIME));
    fprintf(packets, "\n=== BNO085 Stage 3 session %.6f ===\n", seconds(CLOCK_REALTIME));
    message("[LINUX] Opening %s\n", I2C_DEVICE);
    /* [LINUX I2C] C example / Implementation details: open -> I2C_SLAVE.
     * ioctl 성공은 address 설정 성공이며 device ACK의 증거가 아니다. */
    i2c_fd = open(I2C_DEVICE, O_RDWR | O_CLOEXEC);
    if (i2c_fd < 0)
        return system_error("open I2C_DEVICE");
    message("[LINUX] open ... OK\n");
    if (ioctl(i2c_fd, I2C_FUNCS, &functions) < 0)
        return system_error("I2C_FUNCS");
    if ((functions & I2C_FUNC_I2C) == 0) {
        message("[ERROR] adapter does not support plain I2C\n");
        return -1;
    }
    if (BNO085_ADDR != 0x4A && BNO085_ADDR != 0x4B) {
        message("[ERROR] BNO085_ADDR must be 7-bit 0x4A or 0x4B\n");
        return -1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, BNO085_ADDR) < 0)
        return system_error("I2C_SLAVE");
    message("[I2C] Selecting 7-bit address 0x%02X ... OK (ACK 미검증)\n", BNO085_ADDR);
    if (open_interrupt_line() < 0 || drain_pending() < 0)
        return -1;
    message("[BNO085] software reset TX\n");
    if (send_packet(SHTP_CH_EXECUTABLE, reset, sizeof(reset)) < 0)
        return -1;
    /* 새로운 boot epoch에서 RX sequence 기준을 다시 잡는다. */
    memset(rx_seen, 0, sizeof(rx_seen));
    memset(tx_sequence, 0, sizeof(tx_sequence));
    if (wait_boot() < 0 || verify_product() < 0 || log_failed)
        return -1;
    message("[BNO085] Communication verified\n"
            "[STAGE 1] boot + Product ID verified; sensor report enable 없음\n");
    return log_failed ? -1 : 0;
}

int bno085_close(void)
{
    int result = log_failed ? -1 : 0;
    if (int_fd >= 0 && close(int_fd) < 0)
        result = system_error("close H_INTN fd");
    if (i2c_fd >= 0 && close(i2c_fd) < 0)
        result = system_error("close I2C fd");
    int_fd = i2c_fd = -1;
    if (rpy_csv && fclose(rpy_csv) != 0)
        result = system_error("fclose rpy.csv");
    rpy_csv = NULL;
    if (packets && fclose(packets) != 0)
        result = system_error("fclose packets.log");
    packets = NULL;
    if (communication && fclose(communication) != 0) {
        perror("fclose communication.log");
        result = -1;
    }
    communication = NULL;
    if (log_failed)
        fputs("[ERROR] log write failed\n", stderr);
    return result;
}
