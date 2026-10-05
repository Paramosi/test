# BNO085 — SHTP packet과 RPY 전체 해석

**아래 HEX는 규격으로 만든 설명용 예시이며 실제 BNO085 capture가 아니다.** 2026-10-06 Stage 4 확인 시 실제 Pi packet이 없었다. 사용자 요청으로 `logs/communication.log`와 `logs/packets.log`는 빈 file, `logs/rpy.csv`는 header만 준비했다. 실제 evidence는 Pi 실행 후 생성된다. 아래에는 설명용 packet의 전체 해석을 제공하며, 실제 capture 분석은 실제 log 확보 후 수행해야 한다.

## I2C와 SHTP의 경계

`write(fd, bytes, n)`의 `bytes`는 SHTP header부터 시작한다. Linux controller가 I2C START, 7-bit address, R/W, ACK/NACK, STOP을 처리한다. 이 signal들은 C buffer에 포함되지 않는다.

`0x4A`는 7-bit address다. Linux에는 그대로 `0x4A`를 전달한다. bus에서 address와 R/W를 합친 첫 byte는 write=0x94, read=0x95지만 이를 SHTP buffer에 넣으면 안 된다.

근거: [BNO08X §1.2.2.2, Figures 1-15–17, p.15](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf), [Linux I2C C example](https://www.kernel.org/doc/html/latest/i2c/dev-interface.html).

## Header: byte 0–3

| Offset | 의미 | 해석 |
|---|---|---|
| 0 | Length LSB | length의 낮은 8 bit |
| 1 | Length MSB | bit 7은 continuation, 나머지는 length |
| 2 | Channel | SHTP traffic의 목적 |
| 3 | Sequence | channel/direction별 uint8_t counter |

```c
encoded = bytes[0] | (bytes[1] << 8);
length = encoded & 0x7FFF;
continuation = (encoded & 0x8000) != 0;
```

`length`에는 header 4 byte가 포함된다. `encoded=0`은 null header이고 `encoded=0xFFFF`는 reserved error다. SHTP 최대 length는 32766이고 이 구현의 cargo buffer는 1024 byte다. channel 0=SHTP command, 1=executable, 2=SH-2 control을 사용한다.

근거: [SHTP §2.2.1, Figure 2, p.4; §2.3.1, p.5](https://www.ceva-ip.com/wp-content/uploads/Sensor-Hub-Transport-Protocol.pdf), [BNO08X §1.3.1, pp.22–23](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf).

## 예시 1: software reset TX

```text
05 00 01 00 01
```

| Offset | HEX | Binary | 의미 |
|---|---|---|---|
| 0 | 05 | 00000101 | length LSB |
| 1 | 00 | 00000000 | continuation=0, length MSB=0 |
| 2 | 01 | 00000001 | executable channel |
| 3 | 00 | 00000000 | 해당 direction/channel의 첫 sequence |
| 4 | 01 | 00000001 | executable reset command |

`0x0005=5`: header 4 + payload 1. 마지막 `01`은 SH-2 sensor Report ID가 아니라 executable command다. reset complete RX도 channel 1, payload `01`이고 RX sequence는 TX와 독립이다.

근거: [BNO08X §1.3.1, Figure 1-27, p.23](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf).

## 예시 2: boot control cargo

```text
14 00 02 00 | F1 00 84 00 00 00 01 00 00 00 00 00 00 00 00 00
header      | 16-byte SH-2 Initialize Response
```

header의 length는 `0x0014=20`. payload offset 0=`F1`은 Command Response, offset 2=`84`는 unsolicited Initialize, offset 5=`00`은 successful Status다. SHTP sequence, SH-2 command response sequence, command sequence는 서로 다른 field다. 이 예시는 sensor report가 아니므로 Quaternion/RPY를 계산하지 않는다.

근거: [SH-2 §6.3.9, Figure 41, p.44; §6.4.4.2, Figure 53, p.50](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf).

## 예시 3: Product ID request와 response

```text
TX: 06 00 02 00 F9 00
RX: 14 00 02 SS F8 RC MJ MN P0 P1 P2 P3 B0 B1 B2 B3 V0 V1 00 00
```

`SS`는 실제 RX sequence다. 두 `00` reserved field를 포함해 response payload는 16 byte다.

| Payload offset | Field | 조립 |
|---|---|---|
| 0 | Report ID | 0xF8 |
| 1 | Reset Cause | uint8_t |
| 2, 3 | SW Major, Minor | 각각 uint8_t |
| 4–7 | SW Part Number | P0 + (P1<<8) + (P2<<16) + (P3<<24) |
| 8–11 | SW Build Number | B0 + (B1<<8) + (B2<<16) + (B3<<24) |
| 12–13 | SW Patch | V0 + (V1<<8) |
| 14–15 | Reserved | 해석하지 않음 |

예를 들어 `78 56 34 12`는 little-endian으로 `0x12345678`이다. Firmware number를 Quaternion으로 해석하지 않는다.

근거: [SH-2 §§6.3.1–2, Figures 33–34, pp.39–40](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf).

## 긴 cargo의 실제 read 크기와 continuation

이 구현은 처음에 32 byte를 읽는다. length가 5인 reset response라면 payload 1 byte를 사용하고 나머지 27 byte는 padding으로 제외한다. `packets.log`에는 실제 read 결과인 32 byte 전체를 보존한다.

첫 header length=64인 예시라면 payload는 60 byte다.

```text
Transfer 1: header 40 00 00 00 + payload 28 bytes       read 32 bytes
Transfer 2: header 24 80 00 01 + payload 다음 28 bytes  read 32 bytes
Transfer 3: header 08 80 00 02 + payload 마지막 4 bytes read 8 bytes
```

Transfer 2의 encoded length는 `0x8024`: continuation=1, remaining header+payload=36. Transfer 3은 `0x8008`: continuation=1, remaining header+payload=8. 총 I2C RX byte는 72이고 최초 logical length는 64다. 증가한 8 byte는 후속 header 2개다.

모든 후속 transfer에서 header가 반복되므로 byte 0부터 payload로 이어 붙이지 않는다. channel이 바뀌거나 continuation이 없거나 sequence가 연속되지 않으면 incomplete cargo로 실패한다. 이 경로는 실제 센서에서 아직 검증하지 않았으며 규격 기반 mock으로 검증했다.

근거: [SHTP §§2.3.1, 3.4.1](https://www.ceva-ip.com/wp-content/uploads/Sensor-Hub-Transport-Protocol.pdf).

## Log의 한계

TX 성공은 Linux가 요청한 byte 수를 전송했다고 반환한 결과다. 실패한 syscall에서는 실제 bus에 몇 byte가 전송됐는지 알 수 없고 이를 `communication.log`에 명시한다. short positive read/write는 반환된 byte만 raw log에 기록하고 성공 처리하지 않는다.

Terminal의 `[SHTP RX] I2C N bytes`는 transfer byte 수, `[SHTP] length`는 그 transfer의 header field, `[SHTP CARGO] payload`는 조립 후 payload 길이다. 이 세 값은 padding이나 continuation 때문에 다를 수 있다.

## Stage 2: Rotation Vector 1-Hz request

다음도 설명용 synthetic example이다. control TX sequence `SS`는 Stage 1의 Product ID request 이후 보통 01이지만 retry 여부에 따라 달라진다.

```text
15 00 02 SS | FD 05 00 00 00 40 42 0F 00 00 00 00 00 00 00 00 00
SHTP header | Set Feature 17-byte payload
```

| Payload offset | Field | Value |
|---|---|---|
| 0 | command | FD = Set Feature |
| 1 | Feature Report ID | 05 = Rotation Vector |
| 2 | flags | 00 |
| 3–4 | change sensitivity | 0000 |
| 5–8 | report interval | 40 42 0F 00 = 0x000F4240 = 1000000 us |
| 9–12 | batch interval | 00000000 = no added delivery delay |
| 13–16 | sensor configuration | 00000000 |

total length=4+17=21=0x15. 1 Hz는 interval의 의미이며 C program이 매초 command를 보내는 방식이 아니다. enable은 한 번 수행한다.

이후 `06 00 02 SS FE 05`로 Get Feature를 요청한다. 응답은 channel 2의 17-byte `FC 05 ...`이고 offset는 Set Feature와 같다. 실제 interval/flags/batch 등을 요청값과 비교한다. FC response는 sensor data가 아니다.

근거: [SH-2 §§6.5.2–5, Figures 76–79, pp.63–64](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf).

## Stage 2: raw Rotation Vector RX example

```text
17 00 03 10 | FB 0A 00 00 00 | 05 2A 07 02 00 00 00 00 00 00 00 40 00 02
header      | Base Timestamp | Rotation Vector report
```

logical length=23=4+5+14. 첫 read는 32 byte이므로 나머지 9 byte의 padding도 raw log에 남지만 report로 해석하지 않는다.

| Rotation report offset | Field | Example |
|---|---|---|
| 0 | Report ID | 05 |
| 1 | sensor sequence | 2A = 42; SHTP sequence 10과 별개 |
| 2 | Status + Delay upper | 07 |
| 3 | Delay lower | 02 |
| 4–5 | i | 00 00 -> 0 |
| 6–7 | j | 00 00 -> 0 |
| 8–9 | k | 00 00 -> 0 |
| 10–11 | real | 00 40 -> 0x4000 -> signed 16384 |
| 12–13 | accuracy | 00 02 -> 0x0200 -> signed 512 |

```text
status = 0x07 & 0x03 = 3 (Accuracy high)
delay_ticks = ((0x07 & 0xFC) << 6) | 0x02 = 258
delay_us = 258 * 100 = 25800
BaseDelta = 10 ticks -> t_base = t_HINT - 1000 us
t_report relative to t_HINT = (-10 + 258) * 100 = 24800 us
```

`BaseDelta`는 signed 32-bit delta이고 Unix time 또는 sensor uptime이 아니다. `FA` Rebase가 있으면 그 signed delta를 base reference에 더한다. BaseDelta=0x7FFFFFFF는 reserved unavailable 값이므로 정확한 offset를 출력하지 않는다. 이 program은 H_INTN edge 시각을 capture하지 않아 absolute sensor timestamp를 계산하지 않는다.

two's complement의 별도 설명 예시: `00 F0`는 `(0xF0<<8)|0x00=0xF000=61440`, signed 값은 `61440-65536=-4096`이다. byte를 큰 integer pointer로 cast하지 않고 little-endian을 명시적으로 조립한다.

`Status=3`과 `accuracy=512`는 다른 field다. 첫 값은 2-bit quality level이고 두 번째는 heading accuracy estimate의 raw integer다. Stage 2에서는 Quaternion과 accuracy를 Q-format float로 변환하지 않는다.

근거: [SH-2 §6.5.1 Figure 75; §6.5.18.2 Figure 92; §§7.2.1–2 Figures 124–125](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf). Signed accuracy 처리의 CEVA decoder 교차검증은 REFERENCES.md에 있다.

## 여러 report 및 오류 처리

하나의 cargo 안에서 `FB + RV + RV`, 또는 `FB + FA + RV`를 fixed length로 순서대로 해석한다. 알려지지 않은 report가 나타나면 남은 cargo를 raw log에만 남긴다. payload 전체에서 byte 05를 검색해 Quaternion 시작점으로 사용하지 않는다.

잘린 Base/Rebase/RV/control response, Base 없는 RV, configuration mismatch, unexpected reset은 오류다. sensor sequence gap/duplicate는 경고하고 raw 값은 그대로 출력한다. Status=0도 구조적으로 complete한 report이면 raw 값과 Unreliable 표시를 함께 출력한다.

## Stage 3: raw bytes -> Q-format -> Quaternion -> ZYX

위 synthetic RV example을 계속 사용한다. 실제 hardware capture 분석은 log 확보 후 수행해야 한다.

| Field | little-endian 조립 | Signed raw | Conversion |
|---|---|---:|---|
| i -> x | (00<<8)\|00 = 0000 | 0 | 0/16384 = 0 |
| j -> y | (00<<8)\|00 = 0000 | 0 | 0/16384 = 0 |
| k -> z | (00<<8)\|00 = 0000 | 0 | 0/16384 = 0 |
| real -> w | (40<<8)\|00 = 4000 | 16384 | 16384/16384 = 1 |
| accuracy | (02<<8)\|00 = 0200 | 512 | 512/4096 = 0.125 rad |

Representative binary 해석:

```text
length byte 17 = 00010111; MSB byte 00 = 00000000 (continuation=0)
Status byte 07 = 000001 11
                Delay  Status=3
Delay upper=000001, lower=00000010 -> 00000100000010 = 258 ticks
real bytes: 00=00000000, 40=01000000
little-endian word: 01000000 00000000 = 16384 -> Q14 = 1
```

```text
q=(x,y,z,w)=(0,0,0,1)
norm=sqrt(0²+0²+0²+1²)=1  (normalization 없음)
roll  = atan2(2(wx+yz), 1-2(x²+y²)) = atan2(0,1) = 0 deg
pitch = asin(clamp(2(wy-zx),-1,1))  = asin(0)    = 0 deg
yaw   = atan2(2(wz+xy), 1-2(y²+z²)) = atan2(0,1) = 0 deg
```

Q14/Q12 근거: [SH-2 §6.5.18, p.71](https://www.ceva-ip.com/wp-content/uploads/SH-2-Reference-Manual.pdf). Euler 식은 CEVA 정의가 아닌 별도 mathematical derivation이다. [NASA-TM-74839 §2.2 Eq.(15), Appendix A p.A-11](https://ntrs.nasa.gov/api/citations/19770024290/downloads/19770024290.pdf)의 matrix 관계를 적용했으며 convention과 singularity 한계는 REFERENCES.md에 설명했다.

CSV 예시에서 host time을 설명용으로 1700000000.0으로 선택하면 다음과 같다. 실제 실행 결과가 아니다.

```csv
host_time_s,shtp_sequence,sensor_sequence,roll_deg,pitch_deg,yaw_deg,status
1700000000.000000,16,42,0.000000000,0.000000000,0.000000000,3
```

raw Quaternion과 accuracy는 CSV column에 넣지 않는다. Control/boot/feature response는 packet log에만 남고 RPY row를 만들지 않는다. 여러 complete RV를 담은 cargo는 RV마다 별도의 row를 만든다. Zero Quaternion은 raw log에 보존하고 RPY/CSV row를 만들지 않는다.

## 최종 대표 packet: nonzero Yaw의 전체 byte 분석

이 예시는 +90° Z-axis rotation에 가까운 Quaternion을 Q14로 encode한 **synthetic fixture**다. 물리적인 BNO085가 송신했다고 주장하지 않는다. Padding을 포함한 32-byte I2C read 예시:

```text
17 00 03 11 FB 0A 00 00 00 05 2B 07 02 00 00 00
00 41 2D 41 2D 00 02 00 00 00 00 00 00 00 00 00
```

| I2C buffer offset | HEX | Layer / Field | 해석 |
|---|---|---|---|
| 0–1 | 17 00 | SHTP length | 0x0017=23; continuation=0 |
| 2 | 03 | SHTP channel | normal sensor reports |
| 3 | 11 | SHTP sequence | 0x11=17 |
| 4 | FB | SH-2 Base Timestamp ID | 5-byte record 시작 |
| 5–8 | 0A 00 00 00 | Base Delta | signed 10 ticks; base=t_HINT−1000 us |
| 9 | 05 | Rotation Vector ID | 14-byte report 시작 |
| 10 | 2B | sensor sequence | 43; SHTP sequence와 별개 |
| 11 | 07 | Status + Delay upper | status=3, upper=1 |
| 12 | 02 | Delay lower | Delay=(1<<8)+2=258 ticks |
| 13–14 | 00 00 | i / x raw | 0 |
| 15–16 | 00 00 | j / y raw | 0 |
| 17–18 | 41 2D | k / z raw | (0x2D<<8)\|0x41=0x2D41=11585 |
| 19–20 | 41 2D | real / w raw | 11585 |
| 21–22 | 00 02 | accuracy raw | 512 |
| 23–31 | 00 × 9 | padding | raw log에 보존하고 cargo에서는 제외 |

```text
SHTP logical length = 4 + Base 5 + RV 14 = 23
I2C read length = 32; padding = 9
k bytes: 41=01000001, 2D=00101101
little-endian word: 00101101 01000001 = 11585

x=0, y=0
z=11585/16384=0.70709228515625
w=11585/16384=0.70709228515625
norm=0.999979500 (출력값; normalization 없음)
accuracy=512/4096=0.125 rad

Roll  = atan2(0,1) = 0.000000000 deg
Pitch = asin(0)    = 0.000000000 deg
Yaw   = atan2(2*w*z,1-2*z*z) = 89.997650745 deg
```

Q14 rounding과 normalization 부재 때문에 결과는 정확히 90°가 아니다. Source code와 동일한 식으로 example의 byte offsets, signed reconstruction, Q conversion, angle과 CSV 값을 별도로 계산해 대조했다. 이는 문서 계산 검증이며 hardware 성공의 증거가 아니다.

Base/Delay offset는 이전 예시와 같이 `(-10+258)*100=24800 us`다. 이 offset을 host_time_s에 더해 sensor absolute timestamp로 사용하지 않는다. Host time을 설명용으로 1700000001.0으로 선택한 CSV row:

```csv
1700000001.000000,17,43,0.000000000,0.000000000,89.997650745,3
```

## 실제 capture 확보 후 분석할 순서

1. Pi 실행의 communication.log에서 boot/Product ID와 matching FC configuration을 확인한다.
2. 같은 session의 packets.log에서 RX raw bytes, time, length를 선택한다. TX/control response를 RV로 선택하지 않는다.
3. SHTP header를 확인하고 continuation이 있으면 모든 transfer를 모은다. Padding/반복 header를 제외해 cargo를 조립한다.
4. Channel 3의 Base/Rebase/RV를 record length대로 읽고, RV offset 4/6/8/10/12에서 raw를 복원한다.
5. Status/Delay/sequence와 Q14/Q12를 적용한다. norm을 기록하고 ZYX 식으로 degree를 계산한다.
6. communication.log와 rpy.csv의 sequence/angle을 대조한다. 같은 cargo의 여러 RV는 SHTP sequence를 공유할 수 있다.

실제 log가 없으므로 위 실제 capture 대조는 아직 수행하지 않았다.
