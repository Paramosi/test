#ifndef BNO085_H
#define BNO085_H

/* ======================================
 * USER CONFIGURATION
 * ====================================== */
/* Rotation Vector 하나만 1 Hz로 요청한다. */
#define REPORT_INTERVAL_US 1000000
#ifndef PRINT_RAW_PACKET
#define PRINT_RAW_PACKET 1
#endif
#ifndef PRINT_PROTOCOL
#define PRINT_PROTOCOL 1
#endif
#ifndef PRINT_BITS
#define PRINT_BITS 0
#endif
#define BOOT_TIMEOUT_MS 5000
#define READ_TIMEOUT_MS 500
#define MAX_RETRIES 3
#define RECONNECT_DELAY_MS 1000

/* ======================================
 * SYSTEM CONFIGURATION
 * ====================================== */
#ifndef I2C_DEVICE
#define I2C_DEVICE "/dev/i2c-1"
#endif
#ifndef BNO085_ADDR
#define BNO085_ADDR 0x4A
#endif
/* H_INTN -> BCM GPIO17, physical pin 11. gpioinfo로 chip/offset를 확인한다. */
#ifndef GPIO_CHIP
#define GPIO_CHIP "/dev/gpiochip0"
#endif
#ifndef H_INTN_LINE
#define H_INTN_LINE 17
#endif
/* 다음 두 값은 host buffer 정책이며 protocol의 최대 길이가 아니다. */
#define I2C_RX_TRANSFER_BYTES 32
#define RX_CARGO_CAPACITY 1024

/* ======================================
 * BNO085 / SH-2 / SHTP CONSTANTS
 * Defined by official specification.
 * Do not modify as user parameters.
 * ====================================== */
/* [SHTP] 1000-3535 rev 1.10, §2.2.1, Figure 2, p.4. */
#define SHTP_HEADER_BYTES 4
#define SHTP_LENGTH_MASK 0x7FFFu
#define SHTP_CONTINUATION 0x8000u
#define SHTP_ERROR_LENGTH 0xFFFFu
#define SHTP_MAX_LENGTH 32766u
/* [BNO] 1000-3927 rev 1.17, §1.3.1, pp.22–23, Figure 1-27. */
#define SHTP_CHANNEL_COUNT 6
#define SHTP_CH_COMMAND 0
#define SHTP_CH_EXECUTABLE 1
#define SHTP_CH_CONTROL 2
#define EXECUTABLE_RESET 0x01
/* [SHTP] §5.1.1.1, p.14; [SH2] 1000-3625 rev 1.9, §§6.3.1–2,
 * Figures 33–34, pp.39–40; §6.4.4.2, Figure 53, p.50. */
#define SHTP_ADVERTISEMENT 0x00
#define SH2_PRODUCT_ID_REQUEST 0xF9
#define SH2_PRODUCT_ID_RESPONSE 0xF8
#define SH2_PRODUCT_ID_BYTES 16
#define SH2_COMMAND_RESPONSE 0xF1
#define SH2_COMMAND_RESPONSE_BYTES 16
#define SH2_INITIALIZE_UNSOLICITED 0x84

/* [BNO] §1.3.1 p.22: non-wake sensor input channel. */
#define SHTP_CH_REPORTS 3
/* [SH2] 1000-3625 rev 1.9, §§6.5.3–5, Figures 77–79, pp.63–64. */
#define SH2_GET_FEATURE_REQUEST 0xFE
#define SH2_SET_FEATURE_COMMAND 0xFD
#define SH2_GET_FEATURE_RESPONSE 0xFC
#define SH2_FEATURE_BYTES 17
#define SH2_FEATURE_ID_OFFSET 1
#define SH2_FEATURE_FLAGS_OFFSET 2
#define SH2_FEATURE_CHANGE_OFFSET 3
#define SH2_FEATURE_INTERVAL_OFFSET 5
#define SH2_FEATURE_BATCH_OFFSET 9
#define SH2_FEATURE_CONFIG_OFFSET 13
/* [SH2] §6.5.1, Figure 75, pp.62–63; §6.5.18.2, Figure 92, p.71. */
#define SH2_ROTATION_VECTOR 0x05
#define SH2_ROTATION_VECTOR_BYTES 14
#define SH2_SENSOR_SEQUENCE_OFFSET 1
#define SH2_SENSOR_STATUS_OFFSET 2
#define SH2_SENSOR_DELAY_OFFSET 3
#define SH2_STATUS_MASK 0x03u
#define SH2_DELAY_UPPER_MASK 0xFCu
#define SH2_RV_I_OFFSET 4
#define SH2_RV_J_OFFSET 6
#define SH2_RV_K_OFFSET 8
#define SH2_RV_REAL_OFFSET 10
#define SH2_RV_ACCURACY_OFFSET 12
/* [SH2] §6.5.18 p.71: unit Quaternion Q14; heading accuracy Q12 radians. */
#define SH2_RV_QUATERNION_Q 14
#define SH2_RV_ACCURACY_Q 12
/* [SH2] §§7.2.1–2, Figures 124–125, p.92. */
#define SH2_BASE_TIMESTAMP 0xFB
#define SH2_TIMESTAMP_REBASE 0xFA
#define SH2_TIMESTAMP_BYTES 5
#define SH2_TIMESTAMP_DATA_OFFSET 1
#define SH2_BASE_DELTA_UNAVAILABLE 0x7FFFFFFFu
#define SH2_TIME_TICK_US 100

/* 0 = boot와 Product ID 확인 성공, -1 = 실패. 실패 원인은 log에 기록한다. */
int bno085_init(void);
int bno085_enable_rotation_vector(void);
/* >0 = 출력한 Rotation Vector report 수, 0 = timeout/기타 packet, -1 = 오류. */
int bno085_read(void);
int bno085_close(void);
/* signal handler에서 호출 가능: flag만 설정하며 I/O는 하지 않는다. */
void bno085_request_stop(void);

#endif
