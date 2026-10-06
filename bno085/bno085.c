/* Raspberry Pi Zero 2 W + BNO085: Product ID를 한 번 읽고 종료한다.
 * 전원을 유지한 채 다시 실행할 수 있다. 다른 프로그램과 동시에 접근하지 않는다.
 * 빌드: gcc -std=c11 -O2 -Wall -Wextra bno085.c -o bno085
 * 실행: sudo ./bno085
 * 배선: SDA=GPIO2(물리 3), SCL=GPIO3(물리 5), INT=GPIO17(물리 11), 공통 GND.
 * 전원은 센서 보드 규격에 맞춘다. I2C 모드는 PS0=0, PS1=0이다.
 * 참고: CEVA BNO08X Datasheet 1.17 §5.2.1, SHTP 1.10 §3.4.1,
 *       SH-2 Reference Manual 1.9 §§6.3.1-2, 6.4.4.2.
 */

 
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

/* 사용자 설정: 실제 GPIO chip/line은 gpioinfo로 확인한다. */
#define I2C_DEVICE "/dev/i2c-1"
#define BNO085_ADDR 0x4A
#define GPIO_CHIP "/dev/gpiochip0"
#define INT_LINE 17
#define TIMEOUT_SECONDS 5

/* 아래 값은 프로토콜 상수 또는 호스트 버퍼 크기이다. */
enum { CH_COMMAND = 0, CH_EXECUTABLE = 1, CH_CONTROL = 2, CHANNELS = 6 };
enum { COMMAND_RESPONSE = 0xF1, PRODUCT_RESPONSE = 0xF8,
       PRODUCT_REQUEST = 0xF9, FEATURE_RESPONSE = 0xFC };
enum { HEADER_SIZE = 4, TRANSFER_SIZE = 32, PAYLOAD_CAPACITY = 1024,
       COMMAND_RESPONSE_SIZE = 16, PRODUCT_RESPONSE_SIZE = 16 };

static int i2c_fd = -1, int_fd = -1;
static uint8_t tx_sequence[CHANNELS];

struct packet {
    uint8_t channel;
    size_t size;
    uint8_t data[PAYLOAD_CAPACITY];
};

static int open_devices(void);
static int receive_packet(struct packet *out, const struct timespec *deadline);
static int send_packet(uint8_t channel, const uint8_t *data, size_t size);
static int read_product_id(void);

/* Pi의 접근 준비만 수행한다. 실제 센서 응답은 read_product_id()에서 확인한다.
Raspberry Pi 측의 I2C와 INT GPIO를 사용할 준비를 하는 것. */
static int open_devices(void)
{
    if (BNO085_ADDR != 0x4A && BNO085_ADDR != 0x4B) {
        fprintf(stderr, "BNO085 address must be 0x4A or 0x4B\n");
        return -1;
    }
    i2c_fd = open(I2C_DEVICE, O_RDWR | O_CLOEXEC);
    if (i2c_fd < 0) {
        perror("open I2C");
        return -1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, BNO085_ADDR) < 0) {
        perror("I2C_SLAVE");
        return -1;
    }
    int chip_fd = open(GPIO_CHIP, O_RDONLY | O_CLOEXEC);
    if (chip_fd < 0) {
        perror("open GPIO chip");
        return -1;
    }
    struct gpio_v2_line_request request = {0};
    request.offsets[0] = INT_LINE;
    request.num_lines = 1;
    request.config.flags = GPIO_V2_LINE_FLAG_INPUT;
    snprintf(request.consumer, sizeof(request.consumer), "bno085-product-id");
    int result = ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, &request);
    if (result < 0)
        perror("request INT input");
    else
        int_fd = request.fd;
    if (close(chip_fd) < 0) {
        perror("close GPIO chip");
        return -1;
    }
    if (result < 0)
        return -1;
    puts("Devices opened (I2C address selected, INT input ready)");
    return 0;
}

/* 1=완성된 패킷, 0=대기 timeout, -1=오류.
 * deadline은 호출자가 정한다. 다른 패킷이 와도 연장하지 않는다.
 * 이 기한은 GPIO/빈 응답 대기용이다. I2C syscall의 timeout은 커널이 정한다.
 */
static int receive_packet(struct packet *out, const struct timespec *deadline)
{
    uint8_t wire[TRANSFER_SIZE], last_sequence = 0;
    size_t filled = 0, remaining = 0;
    bool first = true;
    memset(out, 0, sizeof(*out));
    for (;;) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
            perror("clock_gettime");
            return -1;
        }
        if (now.tv_sec > deadline->tv_sec ||
            (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec)) {
            if (first)
                return 0;
            fprintf(stderr, "Incomplete SHTP packet: continuation timeout\n");
            return -1;
        }
        struct gpio_v2_line_values values = {0};
        values.mask = 1;
        if (ioctl(int_fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values) < 0) {
            perror("read INT");
            return -1;
        }
        /* ACTIVE_LOW를 쓰지 않으므로 bits=0이 물리적인 LOW이다. */
        if (values.bits & 1u) {
            const struct timespec pause = {0, 1000000L};
            if (nanosleep(&pause, NULL) < 0 && errno != EINTR) {
                perror("INT wait");
                return -1;
            }
            continue;
        }
        size_t requested = first || remaining > TRANSFER_SIZE - HEADER_SIZE
                         ? TRANSFER_SIZE : remaining + HEADER_SIZE;
        /* plain read에는 STOP이 따른다. register 주소나 repeated START는 쓰지 않는다. */
        ssize_t count = read(i2c_fd, wire, requested);
        if (count < 0) {
            perror("I2C read");
            return -1;
        }
        if ((size_t)count != requested) {
            fprintf(stderr, "Short I2C read: %zd/%zu bytes\n", count, requested);
            return -1;
        }
        uint16_t encoded = (uint16_t)((uint16_t)wire[0] | ((uint16_t)wire[1] << 8));
        uint16_t length = (uint16_t)(encoded & 0x7FFFu);
        bool continuation = (encoded & 0x8000u) != 0;
        if (encoded == 0 && first) {
            const struct timespec pause = {0, 1000000L};
            if (nanosleep(&pause, NULL) < 0 && errno != EINTR) {
                perror("empty packet wait");
                return -1;
            }
            continue;
        }
        if (encoded == 0xFFFFu || length <= HEADER_SIZE || wire[2] >= CHANNELS) {
            fprintf(stderr, "Invalid SHTP header: length=0x%04X channel=%u\n",
                    (unsigned)encoded, (unsigned)wire[2]);
            return -1;
        }
        size_t available = (size_t)length - HEADER_SIZE;
        if (first) {
            if (continuation || available > sizeof(out->data)) {
                fprintf(stderr, "Orphan SHTP continuation or payload buffer overflow\n");
                return -1;
            }
            out->channel = wire[2];
            out->size = remaining = available;
        } else if (!continuation || wire[2] != out->channel ||
                   wire[3] != (uint8_t)(last_sequence + 1u) || available != remaining) {
            fprintf(stderr, "SHTP continuation channel/sequence/remaining length mismatch\n");
            return -1;
        }
        size_t copied = requested - HEADER_SIZE;
        if (copied > remaining)
            copied = remaining; /* 끝의 padding은 내용에 포함하지 않는다. */
        memcpy(out->data + filled, wire + HEADER_SIZE, copied);
        filled += copied;
        remaining -= copied;
        last_sequence = wire[3];
        if (remaining == 0)
            return 1;
        first = false;
    }
}

static int send_packet(uint8_t channel, const uint8_t *data, size_t size)
{
    uint8_t wire[TRANSFER_SIZE];
    if (channel >= CHANNELS || size == 0 || size > sizeof(wire) - HEADER_SIZE) {
        fprintf(stderr, "Invalid SHTP transmit channel or size\n");
        return -1;
    }
    size_t length = size + HEADER_SIZE;
    wire[0] = (uint8_t)length;
    wire[1] = (uint8_t)(length >> 8);
    wire[2] = channel;
    wire[3] = tx_sequence[channel];
    memcpy(wire + HEADER_SIZE, data, size);
    ssize_t count = write(i2c_fd, wire, length);
    if (count < 0) {
        perror("I2C write");
        return -1;
    }
    if ((size_t)count != length) {
        fprintf(stderr, "Short I2C write: %zd/%zu bytes\n", count, length);
        return -1;
    }
    ++tx_sequence[channel];
    return 0;
}

/* 받은 내용의 의미를 판단한다. 부팅 확인과 Product ID 확인을 여기에 모은다. */
static int read_product_id(void)
{
    struct packet packet;
    struct timespec deadline;
    bool advertisement = false, reset_done = false, initialized = false;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
        perror("boot clock_gettime");
        return -1;
    }
    deadline.tv_sec += TIMEOUT_SECONDS;
    while (!(advertisement && reset_done && initialized)) {
        int result = receive_packet(&packet, &deadline);
        if (result < 0)
            return -1;
        if (result == 0) {
            /* 부팅 알림은 전원/리셋 직후에만 온다. 재실행에서는 이미 읽었을 수 있다.
             * 알림이 없다는 이유로 성공 판정하지 않고, 실제 Product ID 응답으로 확인한다. */
            if (advertisement || reset_done || initialized) {
                fprintf(stderr, "Incomplete boot: advertisement=%u reset=%u initialization=%u\n",
                        (unsigned)advertisement, (unsigned)reset_done, (unsigned)initialized);
                return -1;
            }
            puts("No startup packets received; requesting Product ID");
            break;
        }
        if (packet.channel == CH_COMMAND && packet.size > 1 && packet.data[0] == 0)
            advertisement = true;
        if (packet.channel == CH_EXECUTABLE && packet.size == 1 && packet.data[0] == 1)
            reset_done = true;
        if (packet.channel == CH_CONTROL) {
            /* F1은 16-byte Command Response이다. 모르는 report의 길이는 추측하지 않는다. */
            for (size_t off = 0; off < packet.size;) {
                const uint8_t *p = packet.data + off;
                if (p[0] != COMMAND_RESPONSE)
                    break;
                if (packet.size - off < COMMAND_RESPONSE_SIZE) {
                    fprintf(stderr, "Truncated boot Command Response\n");
                    return -1;
                }
                if (p[2] == 0x84) { /* unsolicited Initialize */
                    if (p[5] != 0) {
                        fprintf(stderr, "Sensor initialization failed: status=%u\n", (unsigned)p[5]);
                        return -1;
                    }
                    initialized = true;
                }
                off += COMMAND_RESPONSE_SIZE;
            }
        }
    }
    if (advertisement && reset_done && initialized)
        puts("Sensor startup confirmed");

    const uint8_t request[] = {PRODUCT_REQUEST, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
        perror("Product ID clock_gettime");
        return -1;
    }
    deadline.tv_sec += TIMEOUT_SECONDS;
    if (send_packet(CH_CONTROL, request, sizeof(request)) < 0)
        return -1;
    puts("Product ID request sent");
    for (;;) {
        int result = receive_packet(&packet, &deadline);
        if (result < 0)
            return -1;
        if (result == 0) {
            fprintf(stderr, "Product ID response timeout\n");
            return -1;
        }
        if (packet.channel == CH_EXECUTABLE && packet.size == 1 && packet.data[0] == 1) {
            fprintf(stderr, "Sensor reset while waiting for Product ID\n");
            return -1;
        }
        if (packet.channel != CH_CONTROL)
            continue;
        for (size_t off = 0; off < packet.size;) {
            const uint8_t *p = packet.data + off;
            size_t left = packet.size - off, length;
            if (p[0] == PRODUCT_RESPONSE)
                length = PRODUCT_RESPONSE_SIZE;
            else if (p[0] == COMMAND_RESPONSE)
                length = COMMAND_RESPONSE_SIZE;
            else if (p[0] == FEATURE_RESPONSE)
                length = 17;
            else
                break; /* 다음 패킷을 기다린다. payload 안에서 byte 검색을 하지 않는다. */
            if (left < length) {
                fprintf(stderr, "Truncated control report: 0x%02X\n", (unsigned)p[0]);
                return -1;
            }
            if (p[0] == COMMAND_RESPONSE && p[2] == 0x84) {
                fprintf(stderr, "Sensor reinitialized while waiting for Product ID\n");
                return -1;
            }
            if (p[0] == PRODUCT_RESPONSE) {
                unsigned patch = (unsigned)p[12] | ((unsigned)p[13] << 8);
                puts("Product ID response received");
                printf("Firmware version: %u.%u.%u\n", (unsigned)p[2], (unsigned)p[3], patch);
                return 0;
            }
            off += length;
        }
    }
}

int main(void)
{
    int result = 1;
    if (open_devices() == 0 && read_product_id() == 0)
        result = 0;

    /* 정상/실패 모두 열었던 line과 I2C 핸들을 닫는다. */
    if (int_fd >= 0 && close(int_fd) < 0) {
        perror("close INT line");
        result = 1;
    }
    if (i2c_fd >= 0 && close(i2c_fd) < 0) {
        perror("close I2C");
        result = 1;
    }
    if (result == 0 && fflush(stdout) != 0) {
        perror("flush stdout");
        result = 1;
    }
    if (result == 0) {
        if (puts("Communication OK") == EOF || fflush(stdout) != 0) {
            perror("stdout");
            result = 1;
        }
    } else {
        fprintf(stderr, "Communication failed\n");
    }
    return result;
}
