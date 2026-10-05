# Stage 4 — 순수 C BNO085 RPY 프로그램 최종 정리

## 실행 로그 이후 수정 — 2026-10-06

리셋 직후 `length=0` 응답 하나만으로 `boot incomplete`를 반환하던 오류를 수정했다. 빈 응답은 boot 성공으로 세지 않으며, H_INTN을 확인하면서 실제 5초 deadline까지 기다린다. Product ID와 Get Feature 대기도 같은 수신 함수를 사용하므로 빈 응답에서 조기 종료하지 않는다.

프로그램은 Ctrl+C 또는 SIGTERM까지 실행한다. 초기화/설정/수신 오류가 나면 열린 device와 log를 닫고 1초 후 다시 초기화한다. 센서가 예기치 않게 reset되면 같은 경로로 Rotation Vector를 다시 설정한다. 정상적인 500-ms idle에서는 재초기화하지 않는다. 기존 log는 append한다. 실제 센서가 응답하지 않는 동안에는 RPY data를 만들지 않으며 재시도 메시지만 나온다.

Ctrl+C는 signal handler에서 flag만 설정하고 GPIO 대기, 빈 응답 대기, 재시도 대기를 중단한다. 진행 중인 I2C syscall의 종료 시점은 Linux driver에 달려 있다. 정상 cleanup은 exit=0이며 log flush/close 오류가 있으면 exit=1이다. 아래 Stage 4 내용은 원래 검증 기록이고, 이 수정의 실제 Pi/BNO085 수신은 현장 재확인이 필요하다.

검증: Windows GCC 15.2.0에서 production C를 포함한 mock regression의 수신/종료/재시도 **19개 시나리오가 통과**했다. `-Werror`를 포함한 strict warning으로 컴파일했다. 빈 응답 후 boot/Product ID/feature 수신, 실제 deadline, 276-byte 광고의 분할 수신, 정상 idle, 초기화/설정/수신 오류 후 재시도, 대기 중 Ctrl+C, cleanup 오류를 확인했다. Windows에서는 최소한의 Linux/POSIX test 선언을 사용하므로 실제 Linux UAPI/하드웨어 검증은 아니다. 재현: `python3 tests/test_lifecycle.py` 또는 Pi에서 `make test`.

```bash
cd ~/test/bno085_rpy
make
sudo ./bno085_rpy
# Ctrl+C로 종료
```

## 1. 이번 Stage의 목적

Raspberry Pi Zero 2 W와 BNO085 사이의 순수 C 프로그램을 최종 정리했다. Rotation Vector 하나만 1 Hz로 활성화하여 raw bytes -> Quaternion -> ZYX Roll/Pitch/Yaw degree를 추적하고 세 log에 기록한다.

**소스 구현과 문서 정리를 마쳤다. 실제 Pi compile/I2C/1-Hz 수신/물리적인 sensor rotation/실제 packet 분석은 미검증이다.** 사용자가 실제 log가 없다고 확인하여 communication.log와 packets.log는 빈 file, rpy.csv는 header-only file로 준비했다. 설명용 data를 실제 log에 넣지 않았다.

기존 Stage 3의 mock 38개 checks와 2,000개 matrix 비교가 software 검증 기록이다. Stage 4에서는 C source/Makefile의 동작을 바꾸지 않고 문서, packet example, file structure와 기존 검증 범위를 점검했다.

## 2. 전체 흐름 중 현재 위치

```text
Physical orientation -> BNO085 internal fusion -> Rotation Vector / Quaternion
 -> SH-2 report -> SHTP packet -> I2C -> Raspberry Pi hardware
 -> Linux Kernel -> /dev/i2c-1 -> C raw parsing -> Q14 Quaternion
 -> ZYX Euler -> Roll / Pitch / Yaw degree -> terminal + logs + CSV
```

BNO085는 Quaternion을 보낸다. C program이 이를 RPY로 계산한다. Rotation Vector report 하나만 enable하고 추가 sensor fusion/filter/보정은 적용하지 않는다.

## 3. 필요한 용어 설명

| 용어 | 의미 |
|---|---|
| Q14 | signed raw를 2^14=16384로 나누어 해석 |
| Q12 | signed raw를 2^12=4096으로 나누어 해석 |
| Quaternion | q=(x,y,z,w); i/j/k/real에 각각 대응 |
| unit Quaternion | norm=1인 orientation 표현; 실제 Q14 값은 quantization 오차가 있음 |
| norm | sqrt(x²+y²+z²+w²); validation용 출력 |
| ZYX Euler | R=Rz(Yaw) Ry(Pitch) Rx(Roll)의 angle 표현 |
| atan2 | 두 argument의 부호로 quadrant까지 결정하는 inverse tangent |
| asin | [-1,1] argument를 [-90,+90] degree angle로 변환 |
| clamp | asin argument만 [-1,1]에 제한; Quaternion normalization 아님 |
| gimbal lock | Pitch ±90°에서 Roll/Yaw를 각각 유일하게 결정할 수 없는 Euler singularity |
| host_time_s | report 처리 시 CLOCK_REALTIME Unix seconds; sensor sample time 아님 |
| CSV | comma-separated columns로 RPY 기록; raw Quaternion은 넣지 않음 |

SHTP sequence는 transport counter, sensor sequence는 RV counter다. Status는 quality level이고 accuracy는 heading accuracy estimate다.

| 기본 용어 | 의미 |
|---|---|
| userspace / Kernel | C application이 실행되는 영역 / hardware driver를 실행하는 OS 핵심 |
| device file | /dev/i2c-N과 /dev/gpiochipN처럼 Kernel driver에 접근하는 입구 |
| I2C | SDA/SCL을 사용하는 bus protocol; address로 device를 선택 |
| SHTP | header/cargo/channel/sequence로 sensor hub data를 전달하는 transport protocol |
| SH-2 | SHTP cargo 안의 feature/command/sensor report 규칙 |
| header / cargo | 전달 정보 / 실제 report 내용 |
| little-endian | 낮은 byte부터 전달하는 multi-byte integer 순서 |
| H_INTN | sensor가 읽을 data를 준비했음을 알리는 active LOW signal |

## 4. 공식 문서 근거

Page는 인쇄 Page 기준이다. 구현별 상세 field/Revision/PDF viewer Page는 [REFERENCES.md](docs/REFERENCES.md)에 있다.

| Document | Document number | Revision | Section | Page/Table/Figure | Used for |
|---|---|---|---|---|---|
| [BNO08X Datasheet](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf) | 1000-3927 | 1.17 | §§1.2.2, 1.3.1–2, 5.2.1 | pp.14–15, 22–24, 43; Figures 1-12, 1-26–29 | address/interface/reset/boot/channel |
| [SHTP](https://www.ceva-ip.com/wp-content/uploads/Sensor-Hub-Transport-Protocol.pdf) | 1000-3535 | 1.10 | §§2.2.1, 2.3.1, 3.2, 3.4.1 | pp.4–8, Figures 2, 4 | header/continuation/sequence/STOP/padding |
| [SH-2 Reference Manual](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf) | 1000-3625 | 1.9 | §§6.3.1–2, 6.5.1–5, 6.5.18, 7.2.1–2 | pp.39–40, 62–64, 71, 91–92; Figures 33–34, 75–79, 92, 124–125 | Product ID, feature, sensor report, Q14/Q12, timestamp |
| [NASA mathematical reference](https://ntrs.nasa.gov/api/citations/19770024290/downloads/19770024290.pdf) | NASA-TM-74839 / JSC-12960 | July 1977; 별도 revision 없음 | §2.2, Appendix A sequence (10) | p.7 Eq.(15), p.A-11 | Hamilton Quaternion matrix와 ZYX (3,2,1) 관계 |
| [Linux I2C userspace](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html) | 없음 | online latest | C example, Implementation details | HTML; Page/Figure 없음 | open/ioctl/read/write |
| [Linux GPIO userspace](https://www.kernel.org/doc/html/latest/userspace-api/gpio/chardev.html) | 없음 | GPIO v2 | Chip, Line Request | HTML; Page/Figure 없음 | H_INTN readiness |

공식 sensor/transport 규격과 host buffer 정책/사용자 설정을 구분했다. Euler 식은 SH-2의 식이라고 주장하지 않으며 별도 mathematical reference에서 유도했다.

## 5. GitHub 구현 참고

| Repository | File | Function/Symbol | Commit | 무엇을 참고했는가 |
|---|---|---|---|---|
| `adafruit/Adafruit_CircuitPython_BNO08x` | `adafruit_bno08x/i2c.py`, `adafruit_bno08x/__init__.py` | `_send_packet`, `_read_packet`, `enable_feature`, `_parse_sensor_report_data` | `a9aa286c6a0bea3b459a7eac53875658a62e3289` | I2C/SHTP/feature/RV/Q scaling |
| `sparkfun/SparkFun_BNO080_Arduino_Library` | `src/SparkFun_BNO080_Arduino_Library.cpp` | `sendPacket`, `receivePacket`, `setFeatureCommand`, `parseInputReport` | `b9b359fad57cef71ea41a83556ed9221e785f6ed` | header/feature/Quaternion field 배치 |
| `ceva-dsp/sh2` | `sh2_SensorValue.c`, `sh2_util.c`, `sh2.c` | `decodeRotationVector`, `read16`, `sensorhubInputHdlr` | `b514b1e2586ddc195e553dac89fc94c637b25298` | signed raw, Q14/Q12, timestamp 방향 |

각 source의 pinned URL와 공식 규격 대조는 [REFERENCES.md](docs/REFERENCES.md)에 기록했다. Linux UAPI와 Raspberry Pi software I2C overlay의 file/function/Commit도 같은 문서에 있다. 숫자/offset는 공식 규격을 기준으로 사용했다.

## 6. 이번 Stage에서 구현하는 내용

이번 Stage에서는 최종 code/document/source mapping과 대표 packet의 전체 해석을 정리했다. Sensor 기능은 추가하지 않았다. 아래는 완성된 프로그램의 동작이다.

1. 기존 boot/Product ID/1-Hz feature 설정과 raw parsing을 유지한다.
2. x=i/16384, y=j/16384, z=k/16384, w=real/16384를 계산한다.
3. norm과 accuracy_raw/4096 radians를 출력한다.
4. norm으로 Quaternion을 나누지 않고 아래 unit Quaternion 식을 적용한다.
5. RPY degree와 기존 Status/sequence를 CSV 한 행으로 기록한다.
6. CSV header는 새/빈 file에 한 번만 쓴다. 기존 schema/마지막 newline을 확인한다.
7. Ctrl+C에서 세 log와 device resource를 닫는다. log write/flush/close 오류는 실패로 반영한다.

```text
roll  = atan2(2(wx+yz), 1-2(x²+y²))
pitch = asin(clamp(2(wy-zx), -1, 1))
yaw   = atan2(2(wz+xy), 1-2(y²+z²))
degree = radian * 180 / acos(-1)
```

Roll/Yaw 범위는 [-180,+180], Pitch는 [-90,+90] degree다. Yaw offset/0–360 변환/mounting correction/filter는 없다.

norm이 1과 크게 다르면 unit Quaternion 전제를 충족하지 않으므로 angle을 검증된 orientation으로 취급할 수 없다. norm은 그대로 보여주며 임의 threshold/filter를 추가하지 않는다. Zero Quaternion은 raw를 log에 남기고 RPY/CSV row를 만들지 않는다. Status=0과 sequence gap/duplicate는 표시하되 nonzero Quaternion의 RPY row는 보존한다.

1 Hz에서는 500-ms idle timeout이 정상일 수 있다. Set Feature를 재전송하지 않는다. GPIO polling으로 H_INTN edge timestamp를 측정하지 않았으므로 absolute sensor timestamp를 만들지 않는다.

## 7. 코드

```text
bno085_rpy/
  src/main.c
  src/bno085.c
  src/bno085.h
  logs/                 # 두 빈 log와 header-only CSV 준비; Pi 실행에서 data append
    communication.log
    packets.log
    rpy.csv
  docs/REFERENCES.md
  docs/PACKET_FORMAT.md
  Makefile
  README.md
```

실제 파일: [main.c](src/main.c), [bno085.c](src/bno085.c), [bno085.h](src/bno085.h), [Makefile](Makefile). main은 초기화 -> enable -> read loop -> cleanup을 반복하며 오류 후 1초 대기하고 재시도한다. `bno085.c`의 `rotation_to_euler()`가 Q-format/norm/Euler, `log_orientation()`가 terminal/communication/CSV, `open_rpy_csv()`가 header/append 검증을 처리한다. `bno085.h`의 Q14/Q12는 PROTOCOL CONSTANTS다.

```c
while (running) {
    int result = bno085_init();
    if (result == 0 && running)
        result = bno085_enable_rotation_vector();
    while (result == 0 && running) {
        if (bno085_read() < 0 && running)
            result = -1;
    }
    int close_result = bno085_close();
    if (!running)
        return close_result == 0 ? 0 : 1;
    if (reconnect_pause() < 0)
        return 1;
}
```

C11, Linux UAPI와 표준 math library만 사용한다. signal handler용 `bno085_request_stop()`은 flag만 설정한다.

## 8. 컴파일 방법

**Raspberry Pi Linux terminal에서 실행한다.** 프로젝트를 `~/bno085_rpy`로 복사한 경우:

```bash
sudo apt update
sudo apt install build-essential linux-libc-dev i2c-tools gpiod
cd ~/bno085_rpy
make
```

직접 compile하는 방법:

```bash
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
   src/main.c src/bno085.c -o bno085_rpy -lm
```

GPIO v2를 위해 Kernel 5.10 이상과 해당 Linux header가 필요하다. sqrt/atan2/asin/acos를 위해 Makefile에서 -lm으로 표준 math library를 link한다. Windows MinGW는 Pi/Linux 실행 파일을 만드는 compiler가 아니다.

주소가 실제 `0x4B`이면:

```bash
make clean
make CPPFLAGS='-DBNO085_ADDR=0x4B'
```

## 9. Raspberry Pi 실행 방법

GPIO는 BCM numbering이며 physical pin과 구분한다.

| Pi Zero 2 W | BNO085 |
|---|---|
| 3.3 V, physical 1 | board 규격에 맞는 supply; bare IC는 VDD/VDDIO 모두 필요 |
| GND, physical 6 | GND |
| GPIO2/SDA, physical 3 | H_SDA/SDA |
| GPIO3/SCL, physical 5 | H_SCL/SCL |
| GPIO17, physical 11 | H_INTN/INT |

I2C mode는 PS0=0, PS1=0, normal firmware는 BOOTN=HIGH이다. NRST를 LOW에 고정하지 않는다. breakout의 기존 설정과 3.3-V pull-up을 확인한다. 이 프로그램은 executable software reset을 사용한다.

I2C가 꺼져 있다면 `sudo raspi-config`의 Interface Options에서 활성화하고 reboot한다. 실행 전에 다음을 확인한다.

```bash
uname -r
ls /dev/i2c*
i2cdetect -l
i2cdetect -y 1
ls /dev/gpiochip*
gpioinfo
```

scan의 `4a`/`4b`는 address ACK만 확인한다. `UU`이면 Kernel driver의 점유를 확인한다. GPIO chip와 line offset도 실제 H_INTN 연결에 맞아야 한다. 다른 프로그램과 동시에 sensor에 접근하지 않는다.

```bash
cd ~/bno085_rpy
sudo ./bno085_rpy
# 약 1초마다 report를 관찰하며 sensor를 천천히 회전한다.
# Ctrl+C로 종료한다.
echo $?
tail -n 80 logs/communication.log
tail -n 60 logs/packets.log
head -n 6 logs/rpy.csv
```

실행 directory의 세 log에 append한다. Ctrl+C는 host resource를 닫으며 sensor-disable command를 보내지는 않는다. 다음 실행은 reset부터 다시 시작한다. `rpy.csv`에도 append한다. `head -n 6 logs/rpy.csv`로 header와 data를 확인한다.

## 10. 예상 출력

**아래는 설명용 synthetic example이며 실제 Pi 실행 결과가 아니다.** 전체 byte 분석은 [PACKET_FORMAT.md](docs/PACKET_FORMAT.md)에 있다. 현재 준비된 logs에는 이 data가 들어 있지 않다.

```text
[SH-2] Rotation Vector configuration confirmed: 1000000 us
[ROTATION VECTOR RAW REPORT]
I2C RX total=32 bytes
[SHTP]
  length=23 channel=3 sequence=17 last_sequence=17
[SH-2]
  report=Rotation Vector (0x05)
  sensor sequence=43
  Status byte=0x07; status=3 (Accuracy high)
  Delay=((0x07 & 0xFC) << 6) | 0x02 = 258 ticks = 25800 us
[ROTATION VECTOR RAW]
  i/j: 00 00 -> signed 0
  k/real: 41 2D -> (0x2D << 8) | 0x41 = 0x2D41 -> signed 11585
  accuracy: 00 02 -> signed 512
[QUATERNION]
  Q14: x=i/16384=0.000000000 y=j/16384=0.000000000
       z=k/16384=0.707092285 w=real/16384=0.707092285
  norm=0.999979500 (validation only; normalization 없음)
  heading accuracy Q12=0.125000000 rad
[EULER ZYX]
  Roll  = 0.000000 deg
  Pitch = 0.000000 deg
  Yaw   = 89.997651 deg
[STAGE 3] Rotation Vector -> Quaternion -> RPY logged
```

실제 raw 출력은 다섯 field 각각의 bytes/HEX/unsigned/signed 조립 과정을 표시한다. Runtime의 Stage 1/2/3 표시는 boot/raw/RPY 구현 위치를 설명하는 기존 marker이며 Stage 4에서는 바꾸지 않았다.

CSV schema와 설명용 row:

```csv
host_time_s,shtp_sequence,sensor_sequence,roll_deg,pitch_deg,yaw_deg,status
1700000001.000000,17,43,0.000000000,0.000000000,89.997650745,3
```

host time도 설명용 값이다. 실제 실행에서는 report 처리 시각을 기록한다.

## 11. 출력 각 부분의 의미

| 출력 | 의미 |
|---|---|
| feature confirmed | 반환된 RV 설정이 interval=1000000 us 등 요청값과 일치 |
| raw bytes/HEX/signed | sensor byte를 little-endian signed integer로 복원 |
| x/y/z/w | Q14를 적용한 Quaternion |
| norm | unit Quaternion 전제의 validation; 자동 normalization 없음 |
| heading accuracy rad | Q12 accuracy; Status와 별개, CSV에는 넣지 않음 |
| EULER ZYX | 선택한 convention의 degree angle |
| host_time_s | report 처리 시 CLOCK_REALTIME Unix seconds; sensor sample timestamp 아님 |
| shtp_sequence | 조립된 cargo의 첫 header sequence |
| sensor_sequence | RV counter |
| status | 0–3 quality; 0은 Unreliable이며 CSV에도 보존 |

communication.log는 전체 해석, packets.log는 실제 time/direction/length/HEX, rpy.csv는 지정한 일곱 column을 기록한다. CSV에는 raw Quaternion/norm/accuracy를 넣지 않는다.

하나의 batch에 RV가 여러 개 있으면 RV마다 한 행을 쓴다. control/boot/feature response는 RPY 대상이 아니다. packets.log는 padding과 continuation header도 보존한다. `PRINT_RAW_PACKET`을 꺼도 raw file log는 유지한다.

## 12. 성공 조건

### Stage 3에서 수행한 software 검증 — 2026-10-06

GCC 15.2.0 strict warning + -Werror로 production main/driver를 포함한 Windows mock harness를 compile하여 **38개 checks를 통과했다**. 공식 Linux v6.12 UAPI header와 mock syscall/clock을 사용했다. MinGW isfinite macro는 test에서 동일 의미의 builtin으로 대체했다.

검증: Q14/Q12, identity/negative identity, 각 축 양/음 회전, Roll/Yaw 180 boundary, asin 양/음 clamp, near gimbal lock, normalization 부재, zero Quaternion, signed 경계, raw packet -> CSV, 정확한 schema/sequence/time, control/wake/unknown/truncated report 배제, Status=0 보존, 여러 RV, append/header, flush failure, 기존 schema/partial row 오류, full boot/feature/fragmented read, idle, main SIGINT cleanup, 설정 불일치와 boot/short TX/RX regression.

별도로 2,000개 mixed ZYX orientation을 forward Quaternion/rotation matrix와 비교했다. 선택 matrix element의 최대 absolute error=0.000208364282이며 Q14 rounding을 포함한다. Singularity는 별도 fixture로 검증했다. sensor accuracy를 측정한 수치가 아니다.

이는 Linux linkage/Kernel ioctl ABI/실제 bus timing/배선/sensor firmware 검증이 아니다. 모의 log를 hardware evidence로 제공하지 않는다.

### Stage 4에서 수행한 최종 점검

C source/Makefile의 SHA-256이 Stage 3 이후 동일함을 확인했다. 문서의 local links, code fences, 15개 필수 section, configuration/source/file structure/CSV schema를 점검했다. Nonzero Yaw 예시의 32-byte read/23-byte logical length, field offsets, raw values, Q-format, norm, Euler, CSV를 별도로 계산했다. Source 동작을 바꾸지 않아 기존 mock 검증을 반복하지 않았다.

| 검증 대상 | 상태 |
|---|---|
| 순수 C, 단일 RV 1 Hz, raw/Q14/ZYX/CSV 구현 | 구현 및 기존 mock 검증 완료 |
| README/REFERENCES/PACKET_FORMAT 정리 | 완료 |
| logs 초기 file 준비 | 두 빈 log + header-only CSV |
| 실제 Linux compile/I2C/GPIO/1-Hz 수신 | 미검증 |
| sensor 물리 회전과 실제 capture 분석 | 미검증 |

### 실제 Pi에서 필요한 조건

1. Linux compile과 boot/Product ID 확인 성공.
2. FC에서 RV interval=1000000 us 등 요청 설정 확인.
3. 약 1초마다 complete RV와 Quaternion/norm/RPY 출력.
4. sensor rotation에 따른 raw/Quaternion/RPY 변화.
5. 세 log 생성; CSV header는 한 번, RV마다 한 row.
6. Ctrl+C exit=0과 cleanup; 재실행 시 기존 CSV에 append.

**위 hardware 조건은 아직 미확인이다.** Host receipt interval은 scheduling/batching 영향을 받을 수 있다. Axis 방향은 sensor의 실제 orientation과 선택한 convention으로 확인한다.

## 13. 실패 시 확인할 항목

| 증상 | 확인 |
|---|---|
| device file 없음 / permission denied | I2C 활성화, adapter 번호, device/log permission |
| NACK / boot timeout | 전원/GND/address/mode/reset와 H_INTN 배선 |
| GPIO v2 오류 | Kernel/header version, chip/line offset, 다른 consumer 점유 |
| FC timeout / configuration mismatch | channel 2의 raw response, feature ID와 모든 설정 field |
| 500-ms idle 반환 | 1-Hz stream에서 정상 가능; Set Feature를 재전송하지 않음 |
| complete RV 없음 | H_INTN, FC 적용 여부, channel 3 packet과 Base record |
| Status=0 | sensor quality가 Unreliable; raw transport 성공과 구분 |
| sequence gap/duplicate | raw log와 report counter 확인; program은 경고 후 raw 출력 |
| truncated report / continuation mismatch | raw length/header와 공식 규격 대조 |
| unexpected reset/Initialize | 전원 안정성, 실제 reset message; 현재 session을 닫고 재초기화 |
| EIO/packet 손상 | 배선/pull-up/bus timing/clock stretching |
| log write failed | 저장 공간, permission, filesystem |
| rpy.csv schema/partial row 오류 | 기존 file을 확인하고 보존/분리한 뒤 새 log로 실행 |
| zero Quaternion | raw/norm 확인; RPY/CSV row는 생성하지 않음 |
| asin clamp / Pitch ±90° | norm과 Q14 quantization 확인; Roll/Yaw는 singularity에서 유일하지 않음 |

BNO085의 clock stretching과 Pi native I2C 문제의 근거는 [REFERENCES.md](docs/REFERENCES.md)에 있다. native bus에서 반복 실패할 때 software I2C를 확인할 수 있다. Pi에서 `dtoverlay -h i2c-gpio`로 parameter를 확인한 뒤 사용 중인 boot config에 추가한다.

```text
dtoverlay=i2c-gpio,i2c_gpio_sda=23,i2c_gpio_scl=24,i2c_gpio_delay_us=2,bus=3
```

전원을 끄고 SDA=GPIO23/physical16, SCL=GPIO24/physical18로 옮긴다. 적절한 3.3-V pull-up을 확인한다. H_INTN GPIO17은 유지한다. 다시 켜고 실제 adapter 번호를 확인한다. `/dev/i2c-3`인 경우:

```bash
make clean
make CPPFLAGS='-DI2C_DEVICE=\"/dev/i2c-3\"'
sudo ./bno085_rpy
```

`bus=3`은 preferred 번호다. software I2C도 실제 hardware 성공 조건을 다시 확인해야 한다. userspace C에 bit banging을 추가하는 방식이 아니다.

## 14. 이번 Stage에서 완성된 데이터 흐름

```text
Physical orientation -> BNO085 Quaternion -> SH-2 Rotation Vector
 -> SHTP -> I2C -> Pi hardware -> Kernel -> C raw parsing
 -> Q14 x/y/z/w -> norm validation -> ZYX Euler degree
 -> terminal + communication.log + packets.log + rpy.csv
```

Software 구현과 mock 실행은 최종 RPY/CSV까지 확인했다. 실제 Pi/BNO085 경로는 현장에서 검증해야 한다.

## 15. 다음 Stage에서 추가할 내용

계획한 네 Stage의 software 구현과 문서 정리는 여기서 마친다. 추가 Stage나 새로운 sensor 기능을 자동으로 시작하지 않는다.

남은 hardware 검증은 Pi에서 실행하여 communication.log, packets.log, rpy.csv를 확보하는 것이다. 실제 RV 하나의 capture-to-RPY 분석과 현장 성공 판정은 그 evidence가 있어야 완료할 수 있다. PACKET_FORMAT.md의 마지막 항목에 실제 capture 분석 순서를 정리했다.

**Stage 4에서 멈춘다. 실제 hardware 검증과 실제 packet 분석은 남아 있다.**

