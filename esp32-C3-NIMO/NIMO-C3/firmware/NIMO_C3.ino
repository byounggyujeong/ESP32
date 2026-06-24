// ==================================================
// NIMO for ESP32-C3 + TIME & WEATHER (UI FIXED V2)
// Features: MPU6050 Shake, NTP Clock, Open-Meteo, Touch UI
// ==================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "time.h"
#include <math.h>

#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>


#include "WiFiSetup.h"     

/*const char* ssid = "T-Guest";
const char* password = "taostaos";*/
const char* TIMEZONE = "KST-9"; 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH_PIN 6

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu6050;

const unsigned char bmp_clear[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x80,
  0x01, 0x00, 0x01, 0x00,
  0x00, 0x83, 0x82, 0x00,
  0x00, 0x0f, 0xe0, 0x00,
  0x00, 0x3f, 0xf8, 0x00,
  0x00, 0x3f, 0xf8, 0x00,
  0x00, 0x7f, 0xfc, 0x00,
  0x00, 0x7f, 0xfc, 0x00,
  0x00, 0xff, 0xfe, 0x00,
  0x3c, 0xff, 0xfe, 0x78,
  0x00, 0xff, 0xfe, 0x00,
  0x00, 0x7f, 0xfc, 0x00,
  0x00, 0x7f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00,
  0x00, 0x3f, 0xf8, 0x00,
  0x00, 0x0f, 0xe0, 0x00,
  0x00, 0x83, 0x82, 0x00,
  0x01, 0x00, 0x01, 0x00,
  0x02, 0x00, 0x00, 0x80,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};
const unsigned char bmp_clouds[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x03, 0xf0, 0x00,
  0x00, 0x0f, 0xfc, 0x00,
  0x00, 0x1f, 0xfe, 0x00,
  0x00, 0x1f, 0xfe, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x01, 0xff, 0xff, 0xe0,
  0x03, 0xff, 0xff, 0xf0,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf0,
  0x00, 0xff, 0xff, 0x00,
  0x00, 0x1f, 0xf8, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};
const unsigned char bmp_rain[] PROGMEM = {
  0x00, 0x03, 0xf0, 0x00,
  0x00, 0x0f, 0xfc, 0x00,
  0x00, 0x1f, 0xfe, 0x00,
  0x00, 0x1f, 0xfe, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x00, 0x3f, 0xff, 0x00,
  0x01, 0xff, 0xff, 0xe0,
  0x03, 0xff, 0xff, 0xf0,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf8,
  0x07, 0xff, 0xff, 0xf0,
  0x00, 0xff, 0xff, 0x00,
  0x00, 0x1f, 0xf8, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x06, 0x18, 0x61, 0x80,
  0x06, 0x18, 0x61, 0x80,
  0x03, 0x0c, 0x30, 0xc0,
  0x03, 0x0c, 0x30, 0xc0,
  0x01, 0x86, 0x18, 0x60,
  0x01, 0x86, 0x18, 0x60,
  0x30, 0xc3, 0x0c, 0x30,
  0x30, 0xc3, 0x0c, 0x30,
  0x18, 0x61, 0x86, 0x18,
  0x18, 0x61, 0x86, 0x18,
  0x0c, 0x30, 0xc3, 0x0c,
  0x0c, 0x30, 0xc3, 0x0c
};
const unsigned char bmp_tiny_drop[] PROGMEM = { 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x7c, 0x38, 0x00 };
const unsigned char bmp_heart[] PROGMEM = { 0x00, 0x00, 0x0c, 0x60, 0x1e, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_zzz[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char bmp_anger[] PROGMEM = { 0x00, 0x00, 0x11, 0x10, 0x2a, 0x90, 0x44, 0x40, 0x80, 0x20, 0x80, 0x20, 0x44, 0x40, 0x2a, 0x90, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int currentPage = 0; 
bool lastTouchState = false;
unsigned long lastTouchTime = 0;

float calibAngleX = 0, calibAngleY = 0;
float currentRoll = 0, currentPitch = 0;
bool isCalibrated = false;
float lastAccelX = 0, lastAccelY = 0, lastAccelZ = 0;

float temperature = 0;
int humidity = 0;
String weatherMain = "Clear";
String weatherDesc = "Fetching...";
unsigned long lastWeatherUpdate = 0;

#define MOOD_NORMAL 0
#define MOOD_HAPPY 1
#define MOOD_ANGRY 2
#define MOOD_SLEEPY 3
#define MOOD_SAD 4
#define MOOD_LOVE 7

int currentMood = MOOD_HAPPY; 
int baseMood = MOOD_HAPPY;    
unsigned long angryTimer = 0; 

struct Eye { float x, y, pupilX, pupilY; };
Eye leftEye;
Eye rightEye;

const unsigned char* getWeatherIcon(String w) {
  if (w == "Clear") return bmp_clear;
  if (w == "Cloudy") return bmp_clouds;
  if (w == "Rainy" || w == "Snowy") return bmp_rain;
  return bmp_clouds;
}

void wifiStatusToOLED(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setFont(NULL);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 10);
  display.println(line1);
  display.setCursor(0, 25);
  display.println(line2);
  display.display();
}

void connectWiFiAndSyncTime() {
  // 저장된 WiFi로 연결 시도 → 실패 시 NIMO-Setup AP 포털 진입
  //   연결 타임아웃 15초, 포털 대기 180초
  bool ok = wifiSetupBegin("NIMO-Setup", "12345678", 15, 180, wifiStatusToOLED);

  if (ok) {
    wifiStatusToOLED("WiFi Connected!", "Syncing Time...");
    delay(500);
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", TIMEZONE, 1);
    tzset();
    delay(1000);
  } else {
    wifiStatusToOLED("WiFi Failed", "Restarting...");
    delay(2000);
    ESP.restart();   // 저장은 됐으니 재부팅하면 자동 연결 재시도
  }
}

void getWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure(); 
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=35.8703&longitude=128.5911&current=temperature_2m,relative_humidity_2m,weather_code";
  
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    int currIdx = payload.indexOf("\"current\":");
    if (currIdx > 0) {
      int tempIdx = payload.indexOf("\"temperature_2m\":", currIdx) + 17;
      int tempEnd = payload.indexOf(",", tempIdx);
      temperature = payload.substring(tempIdx, tempEnd).toFloat();

      int humiIdx = payload.indexOf("\"relative_humidity_2m\":", currIdx) + 23;
      int humiEnd = payload.indexOf(",", humiIdx);
      humidity = payload.substring(humiIdx, humiEnd).toInt();

      int codeIdx = payload.indexOf("\"weather_code\":", currIdx) + 15;
      int codeEnd = payload.indexOf("}", codeIdx);
      int wCode = payload.substring(codeIdx, codeEnd).toInt();

      if (wCode == 0 || wCode == 1) {
        weatherMain = "Clear";
        weatherDesc = "Sunny";
        baseMood = MOOD_HAPPY;
      } else if (wCode == 2 || wCode == 3) {
        weatherMain = "Cloudy";
        weatherDesc = "Clouds";
        baseMood = MOOD_NORMAL;
      } else if (wCode >= 50 && wCode <= 69) {
        weatherMain = "Rainy";
        weatherDesc = "Rain";
        baseMood = MOOD_SAD;
      } else if (wCode >= 70 && wCode <= 79) {
        weatherMain = "Snowy";
        weatherDesc = "Snow";
        baseMood = MOOD_HAPPY;
      } else {
        weatherMain = "Cloudy";
        weatherDesc = "Bad Weather";
        baseMood = MOOD_SAD;
      }
      
      if (currentMood != MOOD_ANGRY && currentMood != MOOD_LOVE) {
        currentMood = baseMood;
      }
    }
  }
  http.end();
}

void calibrateMPU() {
  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(0, 20);
  display.println("Calibrating MPU...");
  display.display();
  float sumX = 0, sumY = 0;
  for (int i = 0; i < 100; i++) {
    sensors_event_t a, g, temp;
    mpu6050.getEvent(&a, &g, &temp);
    sumX += atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    sumY += atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
    delay(10);
  }
  calibAngleX = sumX / 100.0;
  calibAngleY = sumY / 100.0;
  isCalibrated = true;
}

void updateMotion() {
  if (!isCalibrated) return;

  sensors_event_t a, g, temp;
  mpu6050.getEvent(&a, &g, &temp);

  currentRoll = (atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI) - calibAngleX;
  currentPitch = (atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI) - calibAngleY;

  leftEye.pupilX = constrain(currentRoll / 5, -8, 8);
  leftEye.pupilY = constrain(currentPitch / 5, -6, 6);
  rightEye.pupilX = leftEye.pupilX;
  rightEye.pupilY = leftEye.pupilY;

  float deltaX = abs(a.acceleration.x - lastAccelX);
  float deltaY = abs(a.acceleration.y - lastAccelY);
  float deltaZ = abs(a.acceleration.z - lastAccelZ);
  float shakeIntensity = deltaX + deltaY + deltaZ;

  if (shakeIntensity > 15.0 && currentPage == 0) { 
    currentMood = MOOD_ANGRY;
    angryTimer = millis(); 
  }

  if ((currentMood == MOOD_ANGRY || currentMood == MOOD_LOVE) && (millis() - angryTimer > 3000)) {
    currentMood = baseMood;
  }

  lastAccelX = a.acceleration.x;
  lastAccelY = a.acceleration.y;
  lastAccelZ = a.acceleration.z;
}

// ==================================================
// 화면 그리기 함수
// ==================================================
void drawClockPage() {
  struct tm t;
  if (!getLocalTime(&t)) {
    display.setFont(NULL);
    display.setCursor(30, 30);
    display.print("Syncing...");
    return;
  }
  
  int cx = 12, cy = 12, r = 8;
  display.drawCircle(cx, cy, r, SH110X_WHITE);
  display.fillCircle(cx, cy, 1, SH110X_WHITE);
  display.drawLine(cx, cy, cx, cy - 5, SH110X_WHITE);
  display.drawLine(cx, cy, cx + 3, cy + 3, SH110X_WHITE);

  String ampm = (t.tm_hour >= 12) ? "PM" : "AM";
  int h12 = t.tm_hour % 12;
  if (h12 == 0) h12 = 12;

  display.setFont(NULL);
  display.setCursor(110, 8);
  display.print(ampm);

  // [수정] 텍스트가 점 인디케이터와 겹치지 않게 전체적으로 위로 끌어올림
  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", h12, t.tm_min);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 36);
  display.print(timeStr);

  display.setFont(&FreeSans9pt7b);
  char dateStr[20];
  strftime(dateStr, 20, "%b %d (%a)", &t);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 50); 
  display.print(dateStr);

    // 하단 줄 및 날씨 상태 텍스트
  display.drawLine(0, 54, 128, 54, SH110X_WHITE);
  display.setFont(NULL);
}

void drawWeatherPage() {
  display.drawBitmap(92, 0, getWeatherIcon(weatherMain), 32, 32, SH110X_WHITE);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 16);
  display.print("DAEGU");

  // 온도 텍스트 (가장 큰 폰트)
  display.setFont(&FreeSansBold18pt7b);
  int tempInt = (int)temperature;
  display.setCursor(5, 48); 
  display.print(tempInt);
  
  // 온도 숫자 텍스트의 실제 길이(w)를 계산
  display.setFont(NULL);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(String(tempInt).c_str(), 0, 48, &x1, &y1, &w, &h);
  
  // [핵심 조절 포인트] 온도 기호(°C)의 가로 위치 (더 뒤로 밀기)
  // 숫자 뒤의 간격이 좁다면 아래 24를 30으로 늘리시고, 너무 멀다면 18로 줄이시면 됩니다.
  int symbolX = x1 + w + 24; 
  
  // 도(°) 마크와 C 글자 그리기
  display.fillCircle(symbolX + 10, 28, 2, SH110X_WHITE); 
  display.setCursor(symbolX + 15, 32);
  display.print("C");

  // 습도 아이콘 및 텍스트
  display.drawBitmap(70, 34, bmp_tiny_drop, 8, 8, SH110X_WHITE);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(82, 44); 
  display.print(humidity);
  display.print("%");

  // 하단 줄 및 날씨 상태 텍스트
  display.drawLine(0, 52, 128, 52, SH110X_WHITE);
  display.setFont(NULL);
  //display.setCursor(1, 52);
  //display.print(weatherDesc);
}

void drawEye(Eye& e, int ex, int ey) {
  display.fillRoundRect(ex, ey, 32, 32, 8, SH110X_WHITE);
  int px = ex + 16 + e.pupilX;
  int py = ey + 16 + e.pupilY;
  display.fillCircle(px, py, 7, SH110X_BLACK);
  display.fillCircle(px + 2, py - 2, 2, SH110X_WHITE);

  if (currentMood == MOOD_SLEEPY) {
    display.fillRect(ex, ey, 32, 14, SH110X_BLACK);
  }
  if (currentMood == MOOD_SAD) {
    display.drawLine(ex, ey + 5, ex + 32, ey + 12, SH110X_BLACK);
  }
  if (currentMood == MOOD_ANGRY) {
    display.drawLine(ex, ey + 5, ex + 32, ey, SH110X_BLACK);
  }
}

void drawMouth() {
  int mx = 64;
  int my = 54;
  switch (currentMood) {
    case MOOD_HAPPY:
    case MOOD_LOVE:
      for (int i = -8; i <= 8; i++) {
        int y = my - (i * i / 20);
        display.drawPixel(mx + i, y, SH110X_WHITE);
      }
      break;
    case MOOD_SAD:
      for (int i = -8; i <= 8; i++) {
        int y = my + (i * i / 20);
        display.drawPixel(mx + i, y, SH110X_WHITE);
      }
      break;
    case MOOD_ANGRY:
      display.drawLine(mx - 6, my, mx + 6, my, SH110X_WHITE);
      break;
    default:
      display.drawLine(mx - 4, my, mx + 4, my, SH110X_WHITE);
      break;
  }
}

void drawFacePage() {
  display.setFont(NULL); 
  
  if (currentMood == MOOD_LOVE) { 
    display.drawBitmap(102, 2, bmp_heart, 16, 16, SH110X_WHITE); 
  } else if (currentMood == MOOD_SLEEPY) {
    display.drawBitmap(102, 2, bmp_zzz, 16, 16, SH110X_WHITE);   
  } else if (currentMood == MOOD_ANGRY) {
    display.drawBitmap(56, 0, bmp_anger, 16, 16, SH110X_WHITE);  
  }
  
  drawEye(leftEye, 20, 16);
  drawEye(rightEye, 76, 16);
  drawMouth();
}

void drawPageIndicator() {
  // [수정] 인디케이터를 화면 안쪽으로 살짝 올리고 간격을 넓게(50, 64, 78) 배치하여 안정감 있게 만듦
  int cy = 59; 
  display.fillCircle(50, cy, currentPage == 0 ? 2 : 1, SH110X_WHITE);
  display.fillCircle(64, cy, currentPage == 1 ? 2 : 1, SH110X_WHITE);
  display.fillCircle(78, cy, currentPage == 2 ? 2 : 1, SH110X_WHITE);
}

// ==================================================
// SETUP
// ==================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  if (!display.begin(0x3C, true)) {
    Serial.println("OLED FAIL");
    while (1);
  }
  display.setTextColor(SH110X_WHITE);

  if (mpu6050.begin()) {
    mpu6050.setAccelerometerRange(MPU6050_RANGE_2_G); 
    mpu6050.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu6050.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  calibrateMPU();

  connectWiFiAndSyncTime();
  getWeather();
  lastWeatherUpdate = millis();
}

// ==================================================
// LOOP
// ==================================================
void loop() {
    // ===== 임시 디버그: 버튼 상태 확인 =====
  static unsigned long lastBtnLog = 0;
  if (millis() - lastBtnLog > 500) {
    Serial.print("[BTN] TOUCH_PIN = ");
    Serial.println(digitalRead(TOUCH_PIN));  // 1=안눌림(정상), 0=눌림
    lastBtnLog = millis();
  }
  // =====================================

  updateMotion(); 

  bool pressed = (digitalRead(TOUCH_PIN) == HIGH);
  static bool lastPressed = false;
  static unsigned long pressStart = 0;
  static bool longPressFired = false;

  if (pressed && !lastPressed) {
    // 막 눌리기 시작
    pressStart = millis();
    longPressFired = false;
  }

  if (pressed && !longPressFired && (millis() - pressStart > 3000)) {
    longPressFired = true;
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("Resetting WiFi...");
    display.display();
    delay(1000);
    wifiSetupReset();   // ← WiFiManager wm.resetSettings() 대신
    ESP.restart();
  }

  if (!pressed && lastPressed && !longPressFired) {
    // 짧게 눌렀다가 뗀 경우만 페이지 전환 (롱프레스 발동 안 했을 때)
    if (millis() - lastTouchTime > 200) {
      currentPage++; 
      if (currentPage > 2) { 
        currentPage = 0; 
        currentMood = MOOD_LOVE; 
        angryTimer = millis(); 
      }
      lastTouchTime = millis();
    }
  }

  lastPressed = pressed;

  if (millis() - lastWeatherUpdate > 600000) {
    getWeather();
    lastWeatherUpdate = millis();
  }

  display.clearDisplay();
  switch (currentPage) {
    case 0: drawFacePage(); break;
    case 1: drawClockPage(); break;
    case 2: drawWeatherPage(); break;
  }
  if (currentPage != 0) drawPageIndicator();
  display.display();
  delay(20);
}