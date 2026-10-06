1. open_devices()
/dev/i2c-1 열기
    ↓
센서 주소 0x4A 지정
    ↓
INT에 연결한 GPIO를 입력으로 열기

SCL
Pi → BNO085
"박자 맞추자. 하나, 둘, 셋..."

SDA
Pi ↔ BNO085
"실제 0과 1 데이터를 주고받자."

INT
BNO085 → Pi
"Pi야, 나한테 읽어갈 데이터 있어!"

2. receive_packet()
INT가 LOW인지 확인
    ↓
read()로 바이트 읽기
    ↓
패킷의 길이와 종류 확인
    ↓
완성된 패킷을 돌려주기

3. send_packet()
명령 앞에 SHTP 헤더를 붙이고 write()로 전송

4. read_product_id()
식별 정보 요청 전송
    ↓
receive_packet()으로 응답 대기
    ↓
식별 정보 응답인지 확인
    ↓
펌웨어 버전 등의 정보 출력

5. 
main()
│
├─ open_devices()
│   ├─ open()      : I2C / GPIO 장치 열기
│   └─ ioctl()     : 센서 주소 지정 / INT 입력 설정
│
├─ read_product_id()
│   │
│   ├─ receive_packet()  : 초기 부팅 패킷 수신
│   │                      → 부팅 응답 확인
│   │
│   ├─ send_packet()     : Product ID 요청 전송
│   │   └─ write()
│   │
│   └─ receive_packet()  : 응답 패킷 수신
│       └─ read()         → Product ID 응답 확인·출력
│
└─ close()         : 열린 I2C / GPIO 핸들 닫기