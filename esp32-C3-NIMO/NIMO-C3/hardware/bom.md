# BOM (Bill of Materials) — NIMO

ESP32-C3 기반 데스크 위젯 (표정 / 시계 / 날씨 표시)

## 핵심 부품

| # | 부품 | 사양 | 수량 | 비고 |
|---|------|------|------|------|
| 1 | MCU 보드 | ESP32-C3 Super Mini | 1 | WiFi/BLE 내장, USB-C |
| 2 | OLED 디스플레이 | SH1106 128×64, I2C | 1 | I2C 주소 0x3C (1.3" 기준) |
| 3 | 자이로/가속도 센서 | MPU6050 6축, I2C 모듈 | 1 | 흔들기(shake) 감지용 |
| 4 | 푸시 버튼 | 택트 스위치 | 1 | 페이지 전환 / WiFi 리셋 |
| 5 | USB-C 케이블 | 데이터+전원 | 1 | 펌웨어 업로드 및 전원 |

## 배선 재료

| # | 부품 | 사양 | 수량 | 비고 |
|---|------|------|------|------|
| 6 | 점퍼 와이어 / 배선재 | — | 일괄 | I2C 4선 + 버튼 1선 + 전원 2선 |
| 7 | PCB / 만능기판 | — | 1 | (선택) 고정 배선 시 |

## 핀 연결

| ESP32-C3 핀 | 연결 대상 | 비고 |
|-------------|-----------|------|
| GPIO8 (SDA) | OLED SDA, MPU6050 SDA | I2C 공유 |
| GPIO9 (SCL) | OLED SCL, MPU6050 SCL | I2C 공유 |
| GPIO6       | 버튼 한쪽 | **풀다운** (반대쪽 3.3V 연결) |
| 3.3V        | OLED VCC, MPU6050 VCC, 버튼 | |
| GND         | OLED GND, MPU6050 GND | |
| USB-C       | 5V 전원 입력 | 온보드 |

> **주의 1 — I2C 공유**: OLED와 MPU6050은 SDA/SCL을 병렬로 함께 연결합니다.
> **주의 2 — 버튼 풀다운**: 버튼은 GND가 아니라 **3.3V**에 연결합니다. 코드에서 `INPUT_PULLDOWN` + `HIGH = 눌림`으로 처리합니다.
> I2C 풀업 저항(4.7kΩ)은 OLED·MPU6050 모듈 보드에 내장되어 있어 외부로 추가하지 않습니다.

## 확인 필요 (실제 구성에 맞게 수정)

- [ ] OLED 사이즈: 1.3"(SH1106) 기준으로 작성됨 — 0.96"(SSD1306)면 부품/주소 수정 필요
- [ ] 배터리 구동 여부: 휴대용으로 만들었다면 아래 추가
      - 리튬폴리머 배터리 (예: 3.7V 500mAh)
      - 충전 모듈 (예: TP4056)
      - 전원 스위치
- [ ] 케이스: 3D 프린팅 / 기성품 여부
- [ ] 추가 수동소자(저항·콘덴서) 사용 여부

## 필요 라이브러리 (펌웨어)

- Adafruit GFX Library
- Adafruit SH110X
- Adafruit MPU6050
- Adafruit Unified Sensor
- (WiFi 설정은 외부 라이브러리 없이 자체 구현 — Preferences/WebServer/DNSServer 내장 사용)

## 개발 환경

- Arduino IDE
- ESP32 Arduino Core 3.x (esp32 by Espressif Systems)