#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "WiFiSetup.h"
#include <ESPmDNS.h>

// ==========================================
// PCA9685 인스턴스 (실측 I2C 스캔 주소 기준)
// pwm1 (0x40) : 분 표시
//   채널  0~ 6 : 분 1자리  (S0상단~S6좌하)
//   채널  7~ 8 : 미사용
//   채널  9~15 : 분 10자리 (S0상단~S6좌하)
//
// pwm2 (0x42) : 시 표시
//   채널  0~ 6 : 시 1자리  (S0상단~S6좌하)
//   채널  7~ 8 : 미사용
//   채널  9~15 : 시 10자리 (S0상단~S6좌하)
//
// 세그먼트 위치:
//  ─── S0 (상단 가로)
// |   |
// S5  S1 (좌상, 우상 세로)
//  ─── S2 (중단 가로)
// |   |
// S6  S3 (좌하, 우하 세로)
//  ─── S4 (하단 가로)
// ==========================================
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x42);

Preferences prefs;
WebServer   server(80);

// ==========================================
// 숫자별 서보 펄스값 (0~9), 세그먼트 순서:
// [S0, S1, S2, S3, S4, S5, S6]
// [상단, 우상, 중단, 우하, 하단, 좌상, 좌하]
//
// 실측 기준: 중앙(ON)=375, OFF=150 또는 550
// (펄스 허용범위 120~600, 최대 접힘 120/600)
// ==========================================
int digits[10][7] = {
  {375, 375, 150, 375, 375, 375, 375}, // 0  S2 OFF
  {150, 375, 150, 375, 550, 150, 550}, // 1  S1,S3 ON만
  {375, 375, 375, 150, 375, 150, 375}, // 2  S3,S5 OFF
  {375, 375, 375, 375, 550, 150, 375}, // 3  S5,S6 OFF
  {150, 375, 375, 375, 550, 375, 550}, // 4  S0,S4,S6 OFF
  {375, 150, 375, 375, 375, 375, 550}, // 5  S1,S6 OFF
  {375, 150, 375, 375, 375, 375, 375}, // 6  S1 OFF
  {375, 375, 150, 375, 550, 150, 550}, // 7  S0,S1,S3 ON만
  {375, 375, 375, 375, 375, 375, 375}, // 8  전체 ON
  {375, 375, 375, 375, 375, 375, 550}, // 9  S6 OFF
};

// ==========================================
// 설정 변수
// (WiFi SSID/PW는 WiFiSetup.cpp가 "wifi" 네임스페이스에서 별도 관리)
// ==========================================
bool   use12Hour  = false;
int    sleepStart = 22;
int    sleepEnd   = 7;
int    offsets[28] = {0};
// 오프셋 배열 인덱스:
//  0~ 6 : 분 1자리  S0~S6
//  7~13 : 분 10자리 S0~S6
// 14~20 : 시 1자리  S0~S6
// 21~27 : 시 10자리 S0~S6

// --- 테스트 모드 ---
bool testMode = false;

// ==========================================
// 시간 관리 변수
// ==========================================
int currentHour   = -1, currentMinute = -1, currentSecond = -1;
int prevHour      = -1, prevMinute    = -1;
unsigned long previousMillis = 0;
const long    interval       = 1000;

// ==========================================
// 모터 상태 관리 (그룹별 개별 관리)
// 그룹 0 : 분 1자리  (pwm1 채널 0~6)
// 그룹 1 : 분 10자리 (pwm1 채널 9~15)
// 그룹 2 : 시 1자리  (pwm2 채널 0~6)
// 그룹 3 : 시 10자리 (pwm2 채널 9~15)
// ==========================================
unsigned long groupMoveTime[4] = {0};
bool          groupActive[4]   = {false};
const unsigned long DETACH_DELAY = 400; // 동작완료 후 전력차단 대기(ms)
const unsigned long STEP_DELAY   = 400; // 그룹간 순차 구동 간격(ms)

// ==========================================
// 수면 모드 상태
// ==========================================
bool wasSleeping = false;

// ==========================================
// 비블로킹 업데이트용 상태머신
// ==========================================
enum UpdateState { IDLE, STEP1, STEP2, STEP3, STEP4 };
UpdateState   updateState = IDLE;
unsigned long stepTimer   = 0;

int  pendingDigit[4]  = {0};
bool pendingUpdate[4] = {false};

// ==========================================
// WiFiSetup 상태 콜백 (OLED 없음 → 시리얼로만 출력)
// ==========================================
void onWifiStatus(const char* l1, const char* l2) {
  Serial.printf("[WiFi 상태] %s %s\n", l1, l2);
}

// ==========================================
// 설정 저장 / 불러오기 (NVS, "clock" 네임스페이스)
// ==========================================
void loadSettings() {
  prefs.begin("clock", true);
  use12Hour  = prefs.getBool("12h",  false);
  sleepStart = prefs.getInt("slp_s", 22);
  sleepEnd   = prefs.getInt("slp_e", 7);
  for (int i = 0; i < 28; i++) {
    offsets[i] = prefs.getInt(("o" + String(i)).c_str(), 0);
  }
  prefs.end();
}

void saveSettings() {
  prefs.begin("clock", false);
  prefs.putBool("12h",  use12Hour);
  prefs.putInt("slp_s", sleepStart);
  prefs.putInt("slp_e", sleepEnd);
  for (int i = 0; i < 28; i++) {
    prefs.putInt(("o" + String(i)).c_str(), offsets[i]);
  }
  prefs.end();
}

// ==========================================
// 서보 제어
// ==========================================
void setServoGroup(Adafruit_PWMServoDriver &pwm, int digit,
                   int startServo, int offsetIndexBase) {
  for (int i = 0; i < 7; i++) {
    int finalPulse = constrain(
      digits[digit][i] + offsets[offsetIndexBase + i], 100, 600);
    pwm.setPWM(startServo + i, 0, finalPulse);
  }
}

void detachAllMotors() {
  for (int i = 0; i < 16; i++) {
    pwm1.setPWM(i, 0, 0);
    pwm2.setPWM(i, 0, 0);
  }
  for (int i = 0; i < 4; i++) groupActive[i] = false;
  Serial.println("전체 모터 전력 차단 완료");
}

// 그룹별 DETACH_DELAY(ms) 후 개별 전력 차단
void detachGroupIfDone() {
  unsigned long now = millis();

  if (groupActive[0] && now - groupMoveTime[0] > DETACH_DELAY) {
    for (int i = 0; i < 7; i++)  pwm1.setPWM(i, 0, 0);
    groupActive[0] = false;
    Serial.println("그룹0(분1자리) 전력 차단");
  }
  if (groupActive[1] && now - groupMoveTime[1] > DETACH_DELAY) {
    for (int i = 9; i < 16; i++) pwm1.setPWM(i, 0, 0);
    groupActive[1] = false;
    Serial.println("그룹1(분10자리) 전력 차단");
  }
  if (groupActive[2] && now - groupMoveTime[2] > DETACH_DELAY) {
    for (int i = 0; i < 7; i++)  pwm2.setPWM(i, 0, 0);
    groupActive[2] = false;
    Serial.println("그룹2(시1자리) 전력 차단");
  }
  if (groupActive[3] && now - groupMoveTime[3] > DETACH_DELAY) {
    for (int i = 9; i < 16; i++) pwm2.setPWM(i, 0, 0);
    groupActive[3] = false;
    Serial.println("그룹3(시10자리) 전력 차단");
  }
}

// 88:88 출력 (테스트 모드용)
void showTestPattern() {
  setServoGroup(pwm1, 8,  0,  0);  delay(STEP_DELAY);
  setServoGroup(pwm1, 8,  9,  7);  delay(STEP_DELAY);
  setServoGroup(pwm2, 8,  0, 14);  delay(STEP_DELAY);
  setServoGroup(pwm2, 8,  9, 21);
  for (int i = 0; i < 4; i++) {
    groupMoveTime[i] = millis();
    groupActive[i]   = true;
  }
}

bool isSleepTime() {
  if (currentHour < 0) return false;
  if (sleepStart < sleepEnd) {
    return (currentHour >= sleepStart && currentHour < sleepEnd);
  } else {
    return (currentHour >= sleepStart || currentHour < sleepEnd);
  }
}

// ==========================================
// 비블로킹 상태머신
// ==========================================
void tickUpdateDisplay() {
  if (updateState == IDLE) return;
  if (millis() - stepTimer < STEP_DELAY) return;
  stepTimer = millis();

  switch (updateState) {
    case STEP1:
      if (pendingUpdate[0]) {
        setServoGroup(pwm1, pendingDigit[0], 0, 0); // 분 1자리
        groupMoveTime[0] = millis();
        groupActive[0]   = true;
      }
      updateState = STEP2;
      break;

    case STEP2:
      if (pendingUpdate[1]) {
        setServoGroup(pwm1, pendingDigit[1], 9, 7); // 분 10자리
        groupMoveTime[1] = millis();
        groupActive[1]   = true;
      }
      updateState = STEP3;
      break;

    case STEP3:
      if (pendingUpdate[2]) {
        setServoGroup(pwm2, pendingDigit[2], 0, 14); // 시 1자리
        groupMoveTime[2] = millis();
        groupActive[2]   = true;
      }
      updateState = STEP4;
      break;

    case STEP4:
      if (pendingUpdate[3]) {
        setServoGroup(pwm2, pendingDigit[3], 9, 21); // 시 10자리
        groupMoveTime[3] = millis();
        groupActive[3]   = true;
      }
      updateState = IDLE;
      prevHour    = pendingDigit[2] + pendingDigit[3] * 10;
      prevMinute  = pendingDigit[0] + pendingDigit[1] * 10;
      break;

    default:
      updateState = IDLE;
      break;
  }
}

void requestDisplayUpdate(bool forceUpdate) {
  if (testMode)          return;
  if (isSleepTime())     return;
  if (updateState != IDLE) return;

  int displayHour = currentHour;
  if (use12Hour) {
    displayHour = currentHour % 12;
    if (displayHour == 0) displayHour = 12;
  }

  pendingDigit[0]  = currentMinute % 10;
  pendingDigit[1]  = currentMinute / 10;
  pendingDigit[2]  = displayHour  % 10;
  pendingDigit[3]  = displayHour  / 10;

  pendingUpdate[0] = forceUpdate || (currentMinute % 10 != prevMinute % 10);
  pendingUpdate[1] = forceUpdate || (currentMinute / 10 != prevMinute / 10);
  pendingUpdate[2] = forceUpdate || (displayHour  % 10 != prevHour   % 10);
  pendingUpdate[3] = forceUpdate || (displayHour  / 10 != prevHour   / 10);

  bool anyUpdate = pendingUpdate[0] || pendingUpdate[1] ||
                   pendingUpdate[2] || pendingUpdate[3];
  if (anyUpdate) {
    stepTimer   = millis();
    updateState = STEP1;
    Serial.printf("디스플레이 갱신: %02d:%02d\n", displayHour, currentMinute);
  }
}

// ==========================================
// WiFi 재연결 (저장된 자격증명으로 단순 재시도)
// ==========================================
unsigned long lastWifiCheck = 0;
void checkWifiReconnect() {
  if (millis() - lastWifiCheck < 10000) return;
  lastWifiCheck = millis();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] 연결 끊김 → 재연결 시도...");
    WiFi.reconnect();
  }
}

// ==========================================
// 웹 서버
// ==========================================
void setupWebServer() {

  // --- 메인 설정 페이지 ---
  server.on("/", HTTP_GET, []() {
    String html = R"rawhtml(
<!DOCTYPE html><html lang='ko'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>서보 시계 설정</title>
  <style>
    body { font-family: sans-serif; padding: 20px; max-width: 640px; margin: auto; background: #f0f2f5; }
    h2   { color: #333; }
    .box { background: #fff; padding: 16px; margin-bottom: 16px; border-radius: 10px; box-shadow: 0 1px 4px rgba(0,0,0,0.1); }
    h3   { margin-top: 0; color: #555; border-bottom: 1px solid #eee; padding-bottom: 6px; }
    h4   { color: #666; margin-bottom: 4px; }
    input[type=number]   { width: 54px; padding: 4px; border: 1px solid #ccc; border-radius: 4px; text-align: center; }
    input[type=checkbox] { transform: scale(1.3); margin-left: 6px; }
    select { font-size: 18px; padding: 4px; border-radius: 4px; }
    table { width: 100%; border-collapse: collapse; }
    td    { padding: 4px; text-align: center; font-size: 13px; }
    .btn  { display: inline-block; padding: 10px 28px; font-size: 16px; background: #4a90e2; color: #fff; border: none; border-radius: 6px; cursor: pointer; margin-top: 8px; text-decoration: none; }
    .btn:hover { background: #357abd; }
    label { font-size: 14px; }
    .test-banner { background: #fff3cd; border: 1px solid #e67e22; border-radius: 8px; padding: 10px 16px; margin-bottom: 16px; color: #b45309; font-weight: bold; }
  </style>
</head>
<body>
<h2>⏰ 서보 시계 설정</h2>
)rawhtml";

    if (testMode) {
      html += "<div class='test-banner'>⚠️ 현재 88:88 테스트 모드 — 오프셋 저장 후 복귀 버튼을 누르세요</div>";
    }

    html += "<form action='/save' method='GET'>";
    html += R"rawhtml(
  <div class='box'>
    <h3>기본 설정</h3>
    <label>12시간 모드:
      <input type='checkbox' name='m12' value='1')rawhtml";

    html += String(use12Hour ? " checked" : "");
    html += R"rawhtml(>
    </label><br><br>
    <label>수면 시작(시): <input type='number' name='ss' min='0' max='23' value=')rawhtml";
    html += String(sleepStart);
    html += R"rawhtml('></label> &nbsp;
    <label>수면 종료(시): <input type='number' name='se' min='0' max='23' value=')rawhtml";
    html += String(sleepEnd);
    html += R"rawhtml('></label>
  </div>
  <div class='box'>
    <h3>서보 오프셋 (정밀 보정)</h3>
    <p style='font-size:13px; color:#666;'>
      S0=상단가로 S1=우상세로 S2=중단가로 S3=우하세로<br>
      S4=하단가로 S5=좌상세로 S6=좌하세로<br>
      -100 ~ +100 펄스 (▲▼ 클릭 시 10씩 증감)
    </p>
)rawhtml";

    const char* labels[] = {
      "분 1자리  (채널  0~6)",
      "분 10자리 (채널  9~15)",
      "시 1자리  (채널  0~6)",
      "시 10자리 (채널  9~15)"
    };
    const char* boards[] = { "pwm1", "pwm1", "pwm2", "pwm2" };

    for (int g = 0; g < 4; g++) {
      html += "<h4>" + String(labels[g]) +
              " &nbsp;<small style='color:#aaa;'>" + String(boards[g]) + "</small></h4>";
      html += "<table><tr>";
      for (int i = 0; i < 7; i++) {
        int idx = g * 7 + i;
        const char* segName[] = {"S0상단","S1우상","S2중단","S3우하","S4하단","S5좌상","S6좌하"};
        html += "<td>" + String(segName[i]) +
                "<br><input type='number' name='o" + String(idx) +
                "' min='-100' max='100' step='10' value='" +
                String(offsets[idx]) + "'></td>";
      }
      html += "</tr></table>";
    }

    html += R"rawhtml(
  </div>
  <input class='btn' type='submit' value='💾 설정 저장'>
)rawhtml";

    if (!testMode) {
      html += "&nbsp;<a class='btn' href='/test_on' style='background:#e67e22;'>🔧 88:88 테스트 모드</a>";
    } else {
      html += "&nbsp;<a class='btn' href='/test_off' style='background:#27ae60;'>✅ 시계 모드로 복귀</a>";
    }

    html += R"rawhtml(
</form>
)rawhtml";

    // --- 숫자별 서보 테스트 섹션 ---
    html += R"rawhtml(
<div class='box'>
  <h3>🔢 숫자별 서보 테스트</h3>
  <p style='font-size:13px; color:#888;'>각 자리 숫자를 선택하고 적용 버튼을 누르면 서보가 해당 숫자 모양으로 이동합니다.</p>
  <form action='/digit_test' method='GET'>
  <table style='width:100%; text-align:center;'>
    <tr>
      <th style='padding:8px; color:#555;'>시 10자리</th>
      <th style='padding:8px; color:#555;'>시 1자리</th>
      <th style='padding:8px; color:#555;'>분 10자리</th>
      <th style='padding:8px; color:#555;'>분 1자리</th>
    </tr>
    <tr>
)rawhtml";

    const char* selNames[] = {"d3", "d2", "d1", "d0"};
    for (int s = 0; s < 4; s++) {
      html += "<td><select name='" + String(selNames[s]) + "'>";
      for (int n = 0; n <= 9; n++) {
        html += "<option value='" + String(n) + "'" + (n == 8 ? " selected" : "") + ">" + String(n) + "</option>";
      }
      html += "</select></td>";
    }

    html += R"rawhtml(
    </tr>
  </table>
  <br>
  <input class='btn' type='submit' value='▶ 서보 이동' style='background:#8e44ad;'>
  </form>
</div>
)rawhtml";

    // --- WiFi 재설정 섹션 ---
    html += R"rawhtml(
<div class='box'>
  <h3>📶 WiFi 재설정</h3>
  <p style='font-size:13px; color:#888;'>
    저장된 WiFi 정보를 삭제하고 재부팅합니다.<br>
    재부팅 후 핸드폰에서 <b>ServoClock-Setup</b> AP에 접속해서<br>
    새 WiFi를 설정할 수 있습니다.
  </p>
  <a class='btn' href='/wifi_reset' style='background:#c0392b;'
     onclick="return confirm('WiFi 설정을 초기화하고 재부팅할까요?');">
     ♻️ WiFi 재설정 (재부팅)
  </a>
</div>
</body></html>
)rawhtml";

    server.send(200, "text/html; charset=utf-8", html);
  });

  // --- 설정 저장 ---
  server.on("/save", HTTP_GET, []() {
    use12Hour  = server.hasArg("m12");
    if (server.hasArg("ss")) sleepStart = constrain(server.arg("ss").toInt(), 0, 23);
    if (server.hasArg("se")) sleepEnd   = constrain(server.arg("se").toInt(), 0, 23);
    for (int i = 0; i < 28; i++) {
      if (server.hasArg("o" + String(i))) {
        offsets[i] = constrain(server.arg("o" + String(i)).toInt(), -100, 100);
      }
    }
    saveSettings();

    if (testMode) {
      showTestPattern(); // 오프셋 즉시 반영해서 88:88 재출력
    } else {
      prevHour   = -1;
      prevMinute = -1;
      requestDisplayUpdate(true);
    }

    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Saved");
  });

  // --- 테스트 모드 ON ---
  server.on("/test_on", HTTP_GET, []() {
    testMode   = true;
    prevHour   = -1;
    prevMinute = -1;
    showTestPattern();
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  // --- 테스트 모드 OFF ---
  server.on("/test_off", HTTP_GET, []() {
    testMode   = false;
    prevHour   = -1;
    prevMinute = -1;
    requestDisplayUpdate(true);
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  // --- 숫자 직접 지정 테스트 ---
  server.on("/digit_test", HTTP_GET, []() {
    int d0 = server.hasArg("d0") ? constrain(server.arg("d0").toInt(), 0, 9) : 8;
    int d1 = server.hasArg("d1") ? constrain(server.arg("d1").toInt(), 0, 9) : 8;
    int d2 = server.hasArg("d2") ? constrain(server.arg("d2").toInt(), 0, 9) : 8;
    int d3 = server.hasArg("d3") ? constrain(server.arg("d3").toInt(), 0, 9) : 8;

    setServoGroup(pwm1, d0,  0,  0);  delay(STEP_DELAY); // 분 1자리
    setServoGroup(pwm1, d1,  9,  7);  delay(STEP_DELAY); // 분 10자리
    setServoGroup(pwm2, d2,  0, 14);  delay(STEP_DELAY); // 시 1자리
    setServoGroup(pwm2, d3,  9, 21);                     // 시 10자리

    for (int i = 0; i < 4; i++) {
      groupMoveTime[i] = millis();
      groupActive[i]   = true;
    }

    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  // --- WiFi 재설정 (저장된 자격증명 삭제 후 재부팅) ---
  server.on("/wifi_reset", HTTP_GET, []() {
    server.send(200, "text/html; charset=utf-8",
      "<html><body style='font-family:sans-serif;text-align:center;padding:40px;'>"
      "<h2>WiFi 설정 초기화됨</h2>"
      "<p>5초 후 재부팅됩니다.<br>재부팅 후 <b>ServoClock-Setup</b> AP로 접속해주세요.</p>"
      "</body></html>");
    wifiSetupReset();
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.println("[웹서버] 시작됨");
}

// ==========================================
// Setup
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // ESP32-C3 Super Mini: GPIO8=SDA, GPIO9=SCL
  Wire.begin(8, 9);

  pwm1.begin();
  pwm1.setOscillatorFrequency(27000000);
  pwm1.setPWMFreq(60);

  pwm2.begin();
  pwm2.setOscillatorFrequency(27000000);
  pwm2.setPWMFreq(60);

  loadSettings();

  // --- WiFi 연결 (캡티브 포털 방식) ---
  // 저장된 WiFi 있으면 자동 연결, 없거나 실패하면
  // "ServoClock-Setup" AP를 열어 핸드폰으로 설정 가능
  // (connectTimeoutSec=15, portalTimeoutSec=180 → 3분 동안 설정 대기)
  bool wifiOk = wifiSetupBegin("ServoClock-Setup", nullptr, 15, 180, onWifiStatus);

  if (!wifiOk) {
    Serial.println("[WiFi] 설정 포털 타임아웃 또는 연결 실패 → 재부팅");
    delay(2000);
    ESP.restart();
  }

  Serial.println("\n[WiFi 성공] IP: " + WiFi.localIP().toString());
  setupWebServer();

  if (MDNS.begin("servoclock")) {
    Serial.println("[mDNS] 시작됨 → http://servoclock.local 로도 접속 가능");
  } else {
    Serial.println("[mDNS] 시작 실패 (IP로는 계속 접속 가능)");
  }

  // --- NTP 시간 동기화 (최대 30초) ---
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP 동기화 중...");
  struct tm timeinfo;
  int ntpRetry = 0;
  while (!getLocalTime(&timeinfo) && ntpRetry < 30) {
    delay(1000); Serial.print("."); ntpRetry++;
  }

  if (ntpRetry >= 30) {
    Serial.println("\n[NTP 실패] 시간 없이 동작");
  } else {
    Serial.println("\n시간 동기화 완료!");
    currentHour   = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
    requestDisplayUpdate(true);
  }
}

// ==========================================
// Loop
// ==========================================
void loop() {
  server.handleClient();
  checkWifiReconnect();
  tickUpdateDisplay();
  detachGroupIfDone(); // 그룹별 DETACH_DELAY 후 개별 전력 차단

  unsigned long now = millis();

  // 수면 시간 진입 시 즉시 전력 차단
  if (isSleepTime()) {
    if (!wasSleeping) {
      detachAllMotors();
      wasSleeping = true;
      Serial.println("수면 모드 진입 → 전력 차단");
    }
  } else {
    wasSleeping = false;
  }

  // 1초 타이머: NTP 시간 읽기 + 갱신 요청
  if (now - previousMillis >= interval) {
    previousMillis += interval;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      currentHour   = timeinfo.tm_hour;
      currentMinute = timeinfo.tm_min;
      currentSecond = timeinfo.tm_sec;
      requestDisplayUpdate(false);
    } else {
      Serial.println("[경고] getLocalTime 실패");
    }
  }
}
