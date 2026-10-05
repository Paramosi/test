# BNO085 — 최종 구현 근거와 검증 기록

초기 확인일: 2026-10-05; Stage 2–4 추가 확인일: 2026-10-06. 공식 문서는 protocol의 기준이고 GitHub sensor driver는 구현 교차검증에만 사용했다. 아래 Revision은 실제로 읽은 문서의 Revision이다. 최신 Revision이라는 의미는 아니다. Page는 문서에 인쇄된 번호이며, cover를 포함한 PDF viewer 번호가 다르면 함께 표시했다.

## Official Source: BNO08X

- Document: [BNO08X Datasheet](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf)
- Document Number: 1000-3927
- Revision: 1.17

| Implementation | Section | Table/Figure | Page | Used for |
|---|---|---|---|---|
| I2C mode, boot mode, wiring | §1.2, §1.2.1, §1.2.2 | Figures 1-5, 1-6, 1-11 | 9–15 | PS0/PS1, BOOTN, NRST, H_INTN, SDA/SCL |
| `BNO085_ADDR` | §1.2.2.1 | Figure 1-12 | 14 | 7-bit 0x4A/0x4B, SA0 sampling |
| `data_ready()`, `wait_ready()` | §1.2.2.2 | Figures 1-16, 1-17 및 바로 뒤 paragraph | 15 | clock stretching 필수; BNO085의 empty read 동작 |
| header, channels, reset | §1.3.1 | Figures 1-26, 1-27 | 22–23 | 4-byte header, channel 0–5, executable reset 0x01 |
| `verify_product()` 교차 확인 | §1.3.2 | Figures 1-28, 1-29 | 23–24 | Product ID Request/Response |
| `drain_pending()`, `wait_boot()` | §5.2, §5.2.1 | prose; numbered Figure 없음 | 43 | boot 시 advertisement, executable reset, SH-2 initialization; sensor disabled 상태 |
| hardware startup 참고 | §6.5.3 | Figure 6-9 | 47 | 전원/NRST startup timing; software timeout와 구분 |

`ioctl(I2C_SLAVE)` 성공은 host의 address 설정 완료다. 실제 device와의 통신 증거로 취급하지 않는다. `Product ID`는 firmware 정보이며 chip의 제품명을 문자열로 반환하는 command가 아니다.

## Official Source: SHTP

- Document: [Sensor Hub Transport Protocol](https://www.ceva-ip.com/wp-content/uploads/Sensor-Hub-Transport-Protocol.pdf)
- Document Number: 1000-3535
- Revision: 1.10
- Date: 06/10/2021

| Implementation | Section | Table/Figure | Page (PDF viewer) | Used for |
|---|---|---|---|---|
| `send_packet()`, `receive_cargo()` | §2.2.1 | Figure 2 | 4 (5) | length LSB/MSB, channel, sequence; bit 15; maximum 32766; 0xFFFF error |
| continuation 및 padding | §2.3.1 | prose | 5 (6) | remaining length, sequence 증가, channel 유지, null header, zero padding, startup advertisement 우선 |
| host transfer 정책 | §2.3.2 | prose | 5–6 (6–7) | implementation의 transfer/cargo 제한; local buffer와 protocol limit의 구분 |
| `read()` / `write()` 선택 | §3.2 | prose | 7 (8) | 각 transfer 뒤 STOP; repeated START 금지 |
| H_INTN gating 및 predicted read | §3.4.1 | Figure 4 | 8 (9) | 준비된 cargo를 읽고 길이가 더 길면 추가 read |
| boot advertisement 식별 | §5.1.1.1 | Response 0; Figure 13은 generic response format | 14 (15) | channel 0, response 0의 의미 |
| advertisement 내용 참고 | §5.2 | Figure 16 | 15–16 (16–17) | TLV, channel assignment, implementation limits |

이 최소 driver는 BNO085의 고정 channel을 사용한다. advertisement 전체를 log에 보존하지만 arbitrary sensor hub의 TLV negotiation은 구현하지 않는다. 첫 read는 host 정책인 32 byte다. 최대 cargo buffer는 1024 byte이고 이를 넘으면 truncation하여 성공시키지 않고 실패한다. 이 제한은 SHTP 규격 자체의 최대값이 아니다.

## Official Source: SH-2 transport mapping

- Document: [SH-2 SHTP Reference Manual](https://www.ceva-ip.com/wp-content/uploads/SH-2-SHTP-Reference-Manual.pdf)
- Document Number: 1000-3600
- Revision: 1.6
- Section: §2.2
- Table/Figure: Figure 2 — SH-2 Write Channel Usage
- Page: 4 (PDF viewer 5)
- Implementation: `send_packet()`, `wait_boot()`
- Used for: executable reset/response의 1-byte cargo, control channel 용도. 실제 BNO085 channel 번호는 BNO08X Datasheet §1.3.1에서 확정한다.

이 문서의 Get Feature 항목은 Stage 1에서 사용하지 않았다. Stage 2의 Get Feature는 SH-2 Reference Manual rev 1.9의 Figures 77, 79를 기준으로 구현했다. 상세 대조는 아래 Stage 2 항목에 있다.

## Official Source: SH-2 reports

- Document: [SH-2 Reference Manual](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf)
- Document Number: 1000-3625
- Revision: 1.9
- Date: June 2021

| Implementation | Section | Table/Figure | Page (PDF viewer) | Used for |
|---|---|---|---|---|
| `verify_product()` request | §6.3.1 | Figure 33 | 39 (40) | F9 00, 2-byte request |
| `verify_product()` response | §6.3.2 | Figure 34 | 39–40 (40–41) | F8, 16-byte response; Reset Cause, SW Version, Part/Build Number; little-endian |
| known control report length | §6.3.9 | Figure 41 | 44 (45) | Command Response F1, 16-byte report |
| `wait_boot()` | §6.4.4.2 | Figure 53 | 50 (51) | unsolicited Initialize=0x84; Status offset 5, 0=successful |

Stage 1은 boot/Product ID subset을 사용했다. Stage 2 sensor report와 Stage 3 Q-format/Euler 근거는 아래에 추가했다.

## Official Source: Linux I2C

- Document: [Implementing I2C device drivers in userspace](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html)
- Document Number: 없음; Linux Kernel documentation
- Revision: online `latest`, 확인일 기준 열람; target Pi의 Kernel version은 미확인
- Section: introduction, C example, Implementation details
- Table/Figure: 없음
- Page: HTML이므로 없음
- Implementation: `bno085_init()`, `send_packet()`, `receive_cargo()`
- Used for: `/dev/i2c-N`, `open()`, `I2C_FUNCS`, `I2C_SLAVE`, plain `read()`/`write()`; `I2C_RDWR` combined transaction의 차이.

이 sensor는 register address를 먼저 쓰는 장치가 아니다. SMBus register command를 적용하지 않는다. `I2C_RDWR`를 쓰더라도 한 message만 넣으면 STOP이 가능하지만, 이 구현은 더 단순한 `read()`/`write()`를 선택했다.

## Official Source: Linux GPIO

- Document: [GPIO Character Device Userspace API](https://www.kernel.org/doc/html/latest/userspace-api/gpio/chardev.html)
- Document Number: 없음
- Revision: GPIO v2 API; Linux 5.10에서 처음 추가
- Section: Chip, Line Request, line values
- Table/Figure: 없음
- Page: HTML이므로 없음
- Implementation: `open_interrupt_line()`, `data_ready()`
- Used for: GPIO chip의 line offset, INPUT request, fd 기반 value read.
- API detail: [GPIO_V2_GET_LINE_IOCTL](https://www.kernel.org/doc/html/latest/userspace-api/gpio/gpio-v2-get-line-ioctl.html), [GPIO_V2_LINE_GET_VALUES_IOCTL](https://www.kernel.org/doc/html/latest/userspace-api/gpio/gpio-v2-line-get-values-ioctl.html).

GPIO는 H_INTN readiness 확인에만 사용한다. I2C bus의 SDA/SCL은 Linux I2C driver가 제어한다. userspace에서 직접 bit banging하지 않는다.

## GitHub Cross-check: Adafruit

- Repository: `adafruit/Adafruit_CircuitPython_BNO08x`
- File: `adafruit_bno08x/i2c.py`
- Function / Symbol: `BNO08X_I2C._send_packet`, `_read_header`, `_read_packet`, `_read`
- Commit SHA: `a9aa286c6a0bea3b459a7eac53875658a62e3289`
- Source: [pinned file](https://github.com/adafruit/Adafruit_CircuitPython_BNO08x/blob/a9aa286c6a0bea3b459a7eac53875658a62e3289/adafruit_bno08x/i2c.py)
- Used for: header와 payload를 같은 write에 넣는 방식, 후속 I2C read에도 header가 존재한다는 점 교차검증.
- 공식 대조: SHTP §§2.2.1, 2.3.1, 3.2, 3.4.1.

`_read_packet()`의 RX sequence를 TX array에 넣는 처리와 `_data_ready`의 header polling은 복사하지 않았다. 공식 규격의 독립 direction counter를 사용하고, BNO085 Datasheet의 empty read 동작을 고려해 H_INTN으로 readiness를 확인한다.

## GitHub Cross-check: SparkFun

- Repository: `sparkfun/SparkFun_BNO080_Arduino_Library`
- File: `src/SparkFun_BNO080_Arduino_Library.cpp`
- Function / Symbol: `sendPacket()`, `receivePacket()`, `getData()`, `softReset()`, `resetReason()`
- Commit SHA: `b9b359fad57cef71ea41a83556ed9221e785f6ed`
- Source: [pinned file](https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library/blob/b9b359fad57cef71ea41a83556ed9221e785f6ed/src/SparkFun_BNO080_Arduino_Library.cpp)
- Used for: TX header 구성, channel별 sequence, I2C 후속 transfer의 header 제외, executable reset와 Product ID query.
- 공식 대조: BNO08X §§1.3.1–2, 5.2.1; SHTP §§2.2.1, 2.3.1; SH-2 §§6.3.1–2.

`receivePacket()`에서 continuation bit를 무시하는 처리는 복사하지 않았다. 고정 delay 후 reset 완료를 추정하는 대신 boot message 3종을 확인한다. 32 byte는 이 프로젝트의 host read 크기이며 BNO085 protocol constant가 아니다.

## GitHub Cross-check: Linux UAPI와 bus driver

- Repository: `torvalds/linux`
- Commit SHA: `adc218676eef25575469234709c2d87185ca223a` (`v6.12`)
- Files: `include/uapi/linux/gpio.h`, `include/uapi/linux/i2c-dev.h`, `include/uapi/linux/i2c.h`, `drivers/i2c/busses/i2c-gpio.c`
- Function / Symbol: `gpio_v2_line_request`, `gpio_v2_line_values`, `GPIO_V2_*_IOCTL`, `I2C_FUNCS`, `I2C_SLAVE`, `I2C_FUNC_I2C`, `i2c_gpio_getscl`, `i2c_gpio_probe`
- Used for: 실제 UAPI field 확인; Windows host test에 공식 header를 사용; software I2C driver의 SCL read 지원 확인.
- Sources: [GPIO header](https://github.com/torvalds/linux/blob/adc218676eef25575469234709c2d87185ca223a/include/uapi/linux/gpio.h), [I2C header](https://github.com/torvalds/linux/blob/adc218676eef25575469234709c2d87185ca223a/include/uapi/linux/i2c-dev.h), [i2c-gpio driver](https://github.com/torvalds/linux/blob/adc218676eef25575469234709c2d87185ca223a/drivers/i2c/busses/i2c-gpio.c).

이 header들은 production code에 vendoring하지 않는다. 실제 Pi에서는 OS가 제공하는 Linux header를 include한다. test에서 Linux scalar type과 일부 POSIX syscall을 대체했으므로 Linux ABI/실제 ioctl 검증은 아니다.

## Raspberry Pi 설정 및 clock stretching 참고

- Official Source: [Raspberry Pi Configuration — I2C](https://www.raspberrypi.com/documentation/computers/configuration.html#enable-or-disable-i2c), [GPIO hardware](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#gpio).
- Document Number / Revision / Page: online documentation; 번호와 인쇄 Page 없음.
- Used for: I2C 활성화, GPIO2/SDA와 GPIO3/SCL, GPIO voltage와 pinout 확인.
- Hardware limitation source: [Raspberry Pi Linux issue #254, maintainer discussion](https://github.com/raspberrypi/linux/issues/254). BCM2835 계열 controller의 clock stretching 문제와 software I2C 대안 참고. target Pi의 OS/board에서 문제가 해결됐다고 가정하지 않는다.

### GitHub Cross-check: software I2C overlay

- Repository: `raspberrypi/linux`
- Commit SHA: `a553c4648f68caaa9fe246adb40269af771a0ee7`
- Files: `arch/arm/boot/dts/overlays/README`, `arch/arm/boot/dts/overlays/i2c-gpio-overlay.dts`
- Function / Symbol: `i2c-gpio`, `i2c_gpio_sda`, `i2c_gpio_scl`, `i2c_gpio_delay_us`, `bus`
- Used for: README의 optional software I2C 설정과 GPIO23/24 배선; bus 번호는 preferred 값이라 실제 device file을 다시 확인하도록 안내.
- Sources: [overlay README](https://github.com/raspberrypi/linux/blob/a553c4648f68caaa9fe246adb40269af771a0ee7/arch/arm/boot/dts/overlays/README), [overlay source](https://github.com/raspberrypi/linux/blob/a553c4648f68caaa9fe246adb40269af771a0ee7/arch/arm/boot/dts/overlays/i2c-gpio-overlay.dts).

`kfilipekk/BNO085_IMU_Driver_Spaceflight`는 이번 Stage에서 참조하지 않았다. 위 source의 code를 그대로 복사하거나 외부 driver library를 link하지 않았다.

## Stage 2 — Official Source: SH-2

추가 확인일: 2026-10-06.

- Document: [SH-2 Reference Manual](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf)
- Document Number: 1000-3625
- Revision: 1.9
- Date: June 2021

| Implementation | Section | Table/Figure | Printed Page (PDF viewer) | Used for |
|---|---|---|---|---|
| `bno085_enable_rotation_vector()` | §§6.5.2, 6.5.4 | Figures 76, 78 | 63–64 (64–65) | 17-byte Set Feature payload; FD; feature ID, flags, interval offsets |
| Get Feature query/validation | §§6.5.3, 6.5.5 | Figures 77, 79 | 63–64 (64–65) | FE 05 request; FC response; effective configuration |
| `decode_rotation_report()` | §6.5.1 | Figure 75 | 62–63 (63–64) | sequence, Status bits 1:0, Delay upper bits 7:2 + lower byte; 100-us units |
| raw Quaternion fields | §6.5.18.2 | Figure 92 | 71 (72) | Rotation Vector 0x05; length 14; i/j/k/real offsets 4/6/8/10; accuracy offset 12 |
| batch report walk | §§7.0, 7.2 | Figure 123 | 91 (92) | multiple records; Base Timestamp at batch start |
| Base Timestamp | §7.2.1 | Figure 124 | 91–92 (92–93) | FB, 5-byte record, signed 32-bit Base Delta, 100-us ticks; 0x7FFFFFFF reserved |
| Timestamp Rebase | §7.2.2 | Figure 125 | 92 (93) | FA, 5-byte record, signed Rebase Delta added to base reference |
| future Q-format conversion | §6.5.18 | prose | 71 (72) | default Quaternion Q14 and accuracy Q12/radians; Stage 2 does not scale |

`1000000 us`는 사용자 설정이다. uint32 little-endian encoding은 `0x000F4240 -> 40 42 0F 00`이다. Report ID/offset는 공식 Figure로 확인한 protocol constant다. Feature flags/change sensitivity/batch interval/sensor configuration은 모두 0으로 요청한다.

Timestamp 관계는 `t_base = t_HINT - BaseDelta*100 us`, `t_rebased = t_base + RebaseDelta*100 us`, `t_report = t_rebased + Delay*100 us`다. Rebase가 여러 번 있으면 delta를 누적한다. GPIO polling으로는 정확한 H_INTN assertion edge timestamp를 측정하지 않았으므로 code는 transport reference에 대한 offset만 출력하고 absolute sensor timestamp를 생성하지 않는다.

Get Feature의 effective interval이 1000000 us와 다르면 성공으로 표시하지 않고 실패한다. 설정 응답과 실제 report 수신은 별도 evidence다. 500-ms timeout만으로 1-Hz stream 실패라고 판단하지 않는다.

## Stage 2 — GitHub Cross-check: Adafruit

- Repository: `adafruit/Adafruit_CircuitPython_BNO08x`
- File: `adafruit_bno08x/__init__.py`
- Function / Symbol: `_get_feature_enable_report()`, `enable_feature()`, `_separate_batch()`, `_parse_sensor_report_data()`, `_parse_get_feature_response_report()`, `_AVAIL_SENSOR_REPORTS`
- Commit SHA: `a9aa286c6a0bea3b459a7eac53875658a62e3289`
- Source: [pinned file](https://github.com/adafruit/Adafruit_CircuitPython_BNO08x/blob/a9aa286c6a0bea3b459a7eac53875658a62e3289/adafruit_bno08x/__init__.py)
- Used for: 17-byte command와 interval offset 5, fixed-length batch 분리, signed 16-bit Quaternion parsing, 17-byte FC response.
- Official cross-check: SH-2 Figures 75, 78, 79, 92 및 §7.2.

다른 sensor를 자동 enable하는 dependency logic은 가져오지 않았다. Stage 2에서는 Rotation Vector 하나만 enable하고 raw integer를 출력했다. 현재는 Stage 3의 Q-format/Euler 계산과 함께 출력한다.

## Stage 2 — GitHub Cross-check: SparkFun

- Repository: `sparkfun/SparkFun_BNO080_Arduino_Library`
- File: `src/SparkFun_BNO080_Arduino_Library.cpp`
- Function / Symbol: `setFeatureCommand(uint8_t,uint16_t,uint32_t)`, `parseInputReport()`
- Commit SHA: `b9b359fad57cef71ea41a83556ed9221e785f6ed`
- Source: [pinned file](https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library/blob/b9b359fad57cef71ea41a83556ed9221e785f6ed/src/SparkFun_BNO080_Arduino_Library.cpp)
- Used for: FD field의 little-endian 배치와 Base record 뒤 Quaternion offsets.
- Official cross-check: SH-2 Figures 78, 92, 124.

이 overload는 interval을 millisecond로 받아 1000을 곱한다. 이 프로젝트는 사용자 설정인 microsecond 값 1000000을 직접 encode하므로 추가로 1000을 곱하지 않는다. Base Delta를 absolute sensor clock으로 간주하지 않는다.

## Stage 2 — GitHub Cross-check: CEVA official implementation

- Repository: `ceva-dsp/sh2`
- Commit SHA: `b514b1e2586ddc195e553dac89fc94c637b25298`
- Files: `sh2_SensorValue.c`, `sh2_util.c`, `sh2.c`
- Function / Symbol: `decodeRotationVector()`, `read16()`, `read32()`, `sensorhubInputHdlr()`, `touSTimestamp()`
- Used for: i/j/k/real/accuracy의 signed `read16` 처리, Status/Delay 분리, signed Base/Rebase와 timestamp 방향 교차검증.
- Sources: [SensorValue decoder](https://github.com/ceva-dsp/sh2/blob/b514b1e2586ddc195e553dac89fc94c637b25298/sh2_SensorValue.c), [integer utility](https://github.com/ceva-dsp/sh2/blob/b514b1e2586ddc195e553dac89fc94c637b25298/sh2_util.c), [SH-2 implementation](https://github.com/ceva-dsp/sh2/blob/b514b1e2586ddc195e553dac89fc94c637b25298/sh2.c).
- Official cross-check: SH-2 §§6.5.1, 6.5.18.2, 7.2.1–2.

Reference Manual의 accuracy field에는 별도 unsigned 표기가 없다. CEVA decoder는 accuracy에도 signed `read16()`을 적용한다. 이에 맞춰 signed raw를 표시하고 unsigned/HEX도 함께 남긴다. 정상 heading accuracy는 음수가 아닌 값이어야 하지만 Stage 2에서는 raw 값을 수정하거나 scaling하지 않는다.

## Stage 3 — Official Source: Quaternion Q-format

확인일: 2026-10-06.

- Implementation: `rotation_to_euler()`; `SH2_RV_QUATERNION_Q`, `SH2_RV_ACCURACY_Q`
- Official Source / Document: [SH-2 Reference Manual](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf)
- Document Number: 1000-3625
- Revision: 1.9, June 2021
- Section: §6.5.18, §6.5.18.2
- Table/Figure: Figure 92와 바로 앞 prose
- Page: printed 71 (PDF viewer 72)
- Used for: unit Quaternion Q14, accuracy Q12 radians, i/j/k/real ordering.

`x=i/16384`, `y=j/16384`, `z=k/16384`, `w=real/16384`이며 `accuracy_rad=accuracy/4096`이다. signed raw 값을 double로 바꾼 뒤 나눈다. `norm=sqrt(x*x+y*y+z*z+w*w)`은 validation용이고 Quaternion을 norm으로 나누지 않는다.

SH-2의 이 항목은 Quaternion representation을 정의한다. 이 문서에서 본 프로그램의 Quaternion-to-ZYX Euler 식을 확인하지 못했으므로 아래의 별도 mathematical reference에서 유도한다.

## Stage 3 — Mathematical Reference: Hamilton Quaternion and ZYX

- Implementation: `rotation_to_euler()`
- Document: [Euler Angles, Quaternions, and Transformation Matrices — Working Relationships](https://ntrs.nasa.gov/api/citations/19770024290/downloads/19770024290.pdf)
- Official repository: [NASA NTRS record](https://ntrs.nasa.gov/citations/19770024290)
- Author: D. M. Henderson
- Document Number: NASA-TM-74839; JSC-12960; 77-FM-37
- Revision: separately numbered revision 없음; July 1977 publication
- Section: §2.2, Appendix A, sequence (10) ZYX (3,2,1)
- Table/Figure: Eq.(14)–(15); Appendix A sequence (10)
- Page: printed 7 (PDF viewer 10), A-11 (PDF viewer 25)
- Used for: Hamilton Quaternion matrix와 right-handed ZYX matrix의 관계.

NASA의 scalar-first `(q1,q2,q3,q4)`를 우리 `(w,x,y,z)`로 대응한다. `R=Rz(yaw)*Ry(pitch)*Rx(roll)`로 convention을 명시한다. Unit Quaternion matrix의 elements는 다음과 같다.

```text
R11 = 1 - 2(y²+z²)      R21 = 2(xy+wz)
R31 = 2(xz-wy)          R32 = 2(yz+wx)
R33 = 1 - 2(x²+y²)
roll  = atan2(R32,R33)
pitch = asin(-R31)
yaw   = atan2(R21,R11)
```

NASA Eq.(15)의 diagonal `w²+x²-y²-z²` 등을 `w²+x²+y²+z²=1` 조건으로 위 형태에 쓴 것은 본 구현의 algebraic derivation이다. 위 식을 CEVA가 제공했다고 주장하지 않는다. radians를 `180/acos(-1)`로 곱해 degree로 변환한다. `atan2`의 branch로 Roll/Yaw는 [-180,+180], `asin`으로 Pitch는 [-90,+90]이다. Yaw offset이나 0–360 변환을 하지 않는다.

Q14 quantization으로 `asin` argument가 [-1,1] 밖으로 나가면 scalar argument만 clamp하고 warning을 남긴다. Quaternion normalization은 하지 않는다. Unit norm 전제이므로 norm이 1과 크게 다르면 출력된 angle은 orientation 검증을 통과한 결과로 취급할 수 없다. 임의 norm threshold/filter는 추가하지 않았다. Zero Quaternion은 raw log에 남기고 RPY/CSV row를 생성하지 않는다.

Pitch ±90°에서는 Roll/Yaw를 각각 유일하게 결정할 수 없다. exact/near singularity에서 단순 `atan2/asin`의 결과와 Q14 오차가 크게 나타날 수 있다. 이 최소 구현은 singularity 주변에서 별도 smoothing, angle hold, branch correction을 하지 않는다.

## Stage 3 — GitHub Cross-check: Q scaling

| Repository | File | Function/Symbol | Commit SHA | Used for |
|---|---|---|---|---|
| `ceva-dsp/sh2` | `sh2_SensorValue.c` | `decodeRotationVector`, `SCALE_Q` | `b514b1e2586ddc195e553dac89fc94c637b25298` | signed read16 곱하기 2^-14, accuracy 곱하기 2^-12 |
| `adafruit/Adafruit_CircuitPython_BNO08x` | `adafruit_bno08x/__init__.py` | `_Q_POINT_14_SCALAR`, `_AVAIL_SENSOR_REPORTS`, `_parse_sensor_report_data` | `a9aa286c6a0bea3b459a7eac53875658a62e3289` | RV scale와 four-component unpacking 교차검증 |

Sources: [CEVA pinned decoder](https://github.com/ceva-dsp/sh2/blob/b514b1e2586ddc195e553dac89fc94c637b25298/sh2_SensorValue.c), [Adafruit pinned source](https://github.com/adafruit/Adafruit_CircuitPython_BNO08x/blob/a9aa286c6a0bea3b459a7eac53875658a62e3289/adafruit_bno08x/__init__.py). Official cross-check: SH-2 §6.5.18. Euler conversion은 해당 sensor library에서 복사하지 않았다.

## Stage 3 — Logging and host verification

`rpy.csv`는 사용자 지정 schema다. SH-2 packet format이 아니다. `host_time_s`는 각 report 처리 시의 CLOCK_REALTIME Unix seconds이며 H_INTN edge/sensor sample time이 아니다. `shtp_sequence`는 조립된 cargo의 첫 header sequence다. Fragmented transfer의 마지막 sequence와 전체 raw bytes는 communication/packets log에 남는다. 한 batch에 여러 RV가 있으면 같은 SHTP sequence를 공유하는 여러 CSV row가 생긴다.

Header는 새/빈 file에서 한 번만 작성한다. 기존 schema가 다르거나 마지막 record에 newline이 없으면 file을 보존하고 실패한다. 각 row 뒤 fflush하며 fclose failure도 exit failure로 반영한다. Status=0은 품질 표시와 함께 기록한다. Zero Quaternion은 RPY row에서 제외하고 raw evidence는 보존한다.

Windows host에서 공식 Linux v6.12 UAPI header와 mock syscall/clock을 사용해 production main/driver를 strict GCC 15.2.0 + -Werror로 compile하고 38개 checks를 통과했다. MinGW의 isfinite macro warning을 피하기 위해 harness에서 동일 의미의 compiler builtin을 사용했다; production Linux source는 C11 isfinite를 사용한다. 이는 Linux linkage/Kernel ABI/실제 I2C 검증이 아니다.

2,000개 mixed ZYX orientation을 independent forward half-angle Quaternion과 forward rotation matrix로 비교했다. Q14 rounding을 포함한 선택 matrix element의 최대 absolute error는 0.000208364282였다. Pitch sampling은 [-70,+69] degree로 제한하고 singularity/clamp는 별도 fixture로 검증했다. 이 수치는 algorithm mock 검증이며 sensor accuracy 측정값이 아니다.

## Stage 4 — 최종 점검과 남은 검증

확인일: 2026-10-06. Stage 4는 documentation 정리이며 C source/Makefile의 동작을 바꾸지 않았다. 이미 통과한 Stage 3의 38개 mock checks와 2,000개 matrix 비교를 기존 software 검증 기록으로 유지한다. Stage 4에서 이를 새로 실행했다고 표시하지 않는다.

수행한 점검: 요청된 file structure와 CSV schema, public API, 단일 RV Set Feature, configuration validation, Q-format와 convention, log/cleanup 동작의 문서 대조; local Markdown link와 code fence; nonzero Yaw synthetic packet의 32-byte layout/23-byte logical length/field offsets/signed values/Q conversion/norm/Euler/CSV 계산.

사용자가 실제 packet이 없다고 확인했으며 현재 project logs에 file을 준비하도록 요청했다. communication.log/packets.log는 빈 file, rpy.csv는 header-only file이다. 이 file들의 존재가 hardware 통신 성공을 의미하지 않는다. 실제 Linux compile, GPIO/I2C/clock stretching, 1-Hz RV 수신, 물리적 axis 회전, 실제 capture-to-CSV 대조는 미검증이다.

최종 C source/Makefile의 SHA-256 (software 검증된 Stage 3와 Stage 4의 동일성 점검):

| File | SHA-256 |
|---|---|
| src/main.c | 4f7b54ce5e06f111b741a4f4f0bb7443b1105a3a554e02a31de425643c021488 |
| src/bno085.c | 186f51067abae245d42a11a3bc947d815dacde54da35529931924251310fb85d |
| src/bno085.h | f051bb0928eebf3008697e3a08ca084035358c34b080eee0bed60f1069ef724f |
| Makefile | e475044cfd3c43bbc818f43e1ef907dc14715ef06bc26a580a10a67421d865c5 |
