# NIMO

ESP32-C3 기반 데스크 위젯. OLED에 감정 표현 얼굴, 시계, 실시간 날씨를 표시하며,
흔들면 반응하고 버튼으로 페이지를 전환합니다. WiFi는 첫 부팅 시 스마트폰으로 설정합니다.

## 주요 기능

- **표정 페이지** — MPU6050으로 기울기를 읽어 눈동자가 따라 움직이고, 흔들면 화난 표정으로 반응
- **시계 페이지** — NTP 동기화 기반 실시간 시각 / 날짜 표시
- **날씨 페이지** — Open-Meteo API로 대구 지역 기온·습도·날씨 아이콘 표시
- **WiFi 설정 포털** — 첫 부팅 시 스마트폰으로 WiFi 정보 입력 (코드 수정 불필요)
- **버튼 조작** — 짧게 누르면 페이지 전환, 3초 길게 누르면 WiFi 재설정

## 하드웨어

| 부품 | 사양 |
|------|------|
| MCU | ESP32-C3 Super Mini |
| 디스플레이 | OLED 128×64, I2C (0x3C) |
| 센서 | MPU6050 6축 가속도/자이로 |
| 입력 | 푸시 버튼 ×1 |
| 전원 | USB-C (거치형) |

### 핀 연결

| ESP32-C3 핀 | 연결 대상 | 비고 |
|-------------|-----------|------|
| GPIO8 (SDA) | OLED SDA, MPU6050 SDA | I2C 공유 |
| GPIO9 (SCL) | OLED SCL, MPU6050 SCL | I2C 공유 |
| GPIO6 | 버튼 한쪽 | **풀다운** (반대쪽 3.3V) |
| 3.3V | OLED VCC, MPU6050 VCC, 버튼 | |
| GND | OLED GND, MPU6050 GND | |

> **I2C 공유**: OLED와 MPU6050은 SDA/SCL을 병렬로 함께 연결합니다.
> **버튼 풀다운**: 버튼은 GND가 아니라 **3.3V**에 연결합니다. 코드에서 `INPUT_PULLDOWN` + `HIGH=눌림`으로 처리합니다.

부품 목록(BOM)과 회로도는 [`hardware/`](hardware/) 폴더를 참고하세요.

## 폴더 구조

## 빌드 환경

- **Arduino IDE**
- **ESP32 Arduino Core 3.x** (esp32 by Espressif Systems)
- 보드 선택: `ESP32C3 Dev Module` 또는 보드에 맞는 C3 정의

### 필요 라이브러리

- Adafruit GFX Library
- Adafruit SH110X
- Adafruit MPU6050
- Adafruit Unified Sensor

> WiFi 설정은 외부 라이브러리(WiFiManager 등) 없이 ESP32 내장 기능
> (`Preferences` / `WebServer` / `DNSServer`)으로 자체 구현했습니다.
> Core 3.x에서 WiFiManager의 자격증명 저장 호환성 문제를 우회하기 위함입니다.

### 빌드 설정 주의

- **Tools → Erase All Flash Before Sketch Upload → Disabled**
  (Enabled면 업로드할 때마다 저장된 WiFi 정보가 삭제됩니다)
- **Partition Scheme → Default (NVS 영역 포함)**

## 사용법

### 1. 최초 WiFi 설정

1. 보드에 전원을 연결하면 OLED에 `Connect WiFi to: NIMO-Setup` 표시
2. 스마트폰 WiFi 목록에서 **NIMO-Setup** 접속 (비밀번호: `12345678`)
3. 설정 페이지가 자동으로 열림 (안 뜨면 브라우저에서 `192.168.4.1` 입력)
4. 목록에서 사용할 WiFi 선택 → 비밀번호 입력 → **저장**
5. 자동으로 연결되며, 이후 부팅부터는 저장된 WiFi로 자동 접속

### 2. 평상시 조작

- **버튼 짧게 누르기** → 페이지 전환 (표정 → 시계 → 날씨)
- **버튼 3초 길게 누르기** → WiFi 정보 삭제 후 재시작 (설정 포털 재진입)
- **기기 흔들기** → 표정 페이지에서 화난 표정으로 반응

## 설정 변경

- **날씨 지역**: `NIMO_C3.ino`의 `getWeather()` 안 위도/경도 값 수정
  (기본값: 대구 `latitude=35.8703&longitude=128.5911`)
- **시간대**: `TIMEZONE` 값 수정 (기본 `KST-9`)
- **설정 AP 이름/비밀번호**: `connectWiFiAndSyncTime()`의 `wifiSetupBegin()` 인자 수정

## 라이선스

MIT License
