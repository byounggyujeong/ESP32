// ==================================================
// WiFiSetup.cpp
// ESP32-C3 + Core 3.x 호환 자체구현 WiFi 설정 매니저
// ==================================================
#include "WiFiSetup.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// ---- 내부 전역 ----
static Preferences prefs;
static WebServer   server(80);
static DNSServer   dnsServer;
static WiFiStatusCallback statusCb = nullptr;

static const char* PREF_NAMESPACE = "wifi";   // NVS 네임스페이스
static const char* KEY_SSID = "ssid";
static const char* KEY_PASS = "pass";

static const byte  DNS_PORT = 53;

// 포털에서 입력받은 값 임시 저장
static String newSSID = "";
static String newPASS = "";
static bool   credentialsReceived = false;

// ---- 내부 헬퍼: 콜백으로 상태 표시 ----
static void showStatus(const char* l1, const char* l2) {
  if (statusCb) statusCb(l1, l2);
}

// ---- NVS에서 저장된 자격증명 읽기 ----
static bool loadCredentials(String& ssid, String& pass) {
  prefs.begin(PREF_NAMESPACE, true);   // 읽기전용
  ssid = prefs.getString(KEY_SSID, "");
  pass = prefs.getString(KEY_PASS, "");
  prefs.end();
  return ssid.length() > 0;
}

// ---- NVS에 자격증명 저장 ----
static void saveCredentials(const String& ssid, const String& pass) {
  prefs.begin(PREF_NAMESPACE, false);  // 쓰기 가능
  prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASS, pass);
  prefs.end();
  Serial.print("[WiFiSetup] Saved SSID: ");
  Serial.println(ssid);
}

// ---- 공개 함수: 저장된 자격증명 삭제 ----
void wifiSetupReset() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.clear();
  prefs.end();
  Serial.println("[WiFiSetup] Credentials cleared.");
}

// ---- 공개 함수: 저장된 SSID 존재 여부 ----
bool wifiSetupHasCredentials() {
  String s, p;
  return loadCredentials(s, p);
}

// ---- 저장된 WiFi로 연결 시도 ----
static bool tryConnect(const String& ssid, const String& pass, uint32_t timeoutSec) {
  if (ssid.length() == 0) return false;

  Serial.print("[WiFiSetup] Connecting to: ");
  Serial.println(ssid);
  showStatus("Connecting to", ssid.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutSec * 1000UL) {
      Serial.println("[WiFiSetup] Connect timeout.");
      return false;
    }
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("[WiFiSetup] Connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

// ==================================================
// 웹 포털 핸들러
// ==================================================

// 설정 페이지 HTML 생성 (주변 WiFi 스캔 결과 포함)
static String buildPortalPage() {
  String html = F(
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<meta charset='UTF-8'>"
    "<title>NIMO WiFi Setup</title>"
    "<style>"
    "body{font-family:sans-serif;background:#1a1a1a;color:#eee;margin:0;padding:20px;}"
    "h2{text-align:center;}"
    ".card{max-width:400px;margin:0 auto;background:#2a2a2a;padding:20px;border-radius:12px;}"
    "input,select{width:100%;padding:12px;margin:8px 0;box-sizing:border-box;"
    "border:1px solid #444;border-radius:8px;background:#1a1a1a;color:#eee;font-size:16px;}"
    "button{width:100%;padding:14px;margin-top:12px;border:none;border-radius:8px;"
    "background:#4a90d9;color:#fff;font-size:16px;font-weight:bold;}"
    ".net{padding:10px;border-bottom:1px solid #444;cursor:pointer;}"
    ".net:hover{background:#333;}"
    "small{color:#999;}"
    "</style></head><body>"
    "<div class='card'>"
    "<h2>NIMO WiFi Setup</h2>"
    "<form action='/save' method='POST'>"
    "<label>WiFi 이름 (SSID)</label>"
    "<input type='text' name='ssid' id='ssid' placeholder='네트워크 선택 또는 입력' required>"
    "<label>비밀번호</label>"
    "<input type='password' name='pass' placeholder='WiFi 비밀번호'>"
    "<button type='submit'>저장 후 연결</button>"
    "</form>"
    "<p><small>아래 목록에서 네트워크를 누르면 자동 입력됩니다.</small></p>"
  );

  // 주변 WiFi 스캔
  int n = WiFi.scanNetworks();
  if (n == 0) {
    html += F("<p><small>네트워크를 찾지 못했습니다.</small></p>");
  } else {
    for (int i = 0; i < n; i++) {
      String s = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      html += "<div class='net' onclick=\"document.getElementById('ssid').value='";
      html += s;
      html += "'\">";
      html += s;
      html += " <small>(";
      html += rssi;
      html += " dBm)</small></div>";
    }
  }
  WiFi.scanDelete();

  html += F("</div></body></html>");
  return html;
}

// 루트 페이지
static void handleRoot() {
  server.send(200, "text/html", buildPortalPage());
}

// 저장 요청 처리
static void handleSave() {
  newSSID = server.arg("ssid");
  newPASS = server.arg("pass");

  String resp = F(
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<meta charset='UTF-8'>"
    "<style>body{font-family:sans-serif;background:#1a1a1a;color:#eee;"
    "text-align:center;padding:40px;}</style></head><body>"
    "<h2>저장되었습니다</h2>"
    "<p>NIMO가 재시작하며<br>입력하신 WiFi에 연결합니다.</p>"
    "<p>이 창은 닫으셔도 됩니다.</p>"
    "</body></html>"
  );
  server.send(200, "text/html", resp);

  credentialsReceived = true;   // 메인 루프에 저장 완료 신호
}

// 캡티브 포털: 어떤 주소로 와도 설정 페이지로 리다이렉트
static void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// ==================================================
// 설정 포털(AP 모드) 실행
// ==================================================
static bool runConfigPortal(const char* apName, const char* apPassword,
                            uint32_t portalTimeoutSec) {
  Serial.println("[WiFiSetup] Starting config portal (AP mode)...");
  showStatus("Connect WiFi to:", apName);

  WiFi.mode(WIFI_AP);
  delay(100);
  if (apPassword && strlen(apPassword) >= 8) {
    WiFi.softAP(apName, apPassword);
  } else {
    WiFi.softAP(apName);   // 오픈 AP
  }
  delay(300);

  IPAddress apIP = WiFi.softAPIP();
  Serial.print("[WiFiSetup] AP IP: ");
  Serial.println(apIP);

  // 캡티브 포털용 DNS (모든 도메인을 AP IP로)
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();

  credentialsReceived = false;
  uint32_t start = millis();

  // 포털 루프
  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();

    // 사용자가 저장 버튼 누름 → 잠깐 대기 후 저장/연결
    if (credentialsReceived) {
      delay(500);                 // 응답 페이지 전송 완료 시간 확보
      saveCredentials(newSSID, newPASS);

      server.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      delay(200);

      // 새 자격증명으로 바로 연결 시도
      showStatus("Saved! Connecting", newSSID.c_str());
      if (tryConnect(newSSID, newPASS, 15)) {
        return true;
      } else {
        // 연결 실패해도 저장은 됐으니 재부팅 후 재시도하도록 false 반환
        Serial.println("[WiFiSetup] Saved but connect failed. Will restart.");
        return false;
      }
    }

    // 포털 타임아웃
    if (portalTimeoutSec > 0 && (millis() - start > portalTimeoutSec * 1000UL)) {
      Serial.println("[WiFiSetup] Portal timeout.");
      server.stop();
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      return false;
    }

    delay(10);
  }
}

// ==================================================
// 공개 메인 함수
// ==================================================
bool wifiSetupBegin(const char* apName,
                    const char* apPassword,
                    uint32_t connectTimeoutSec,
                    uint32_t portalTimeoutSec,
                    WiFiStatusCallback cb) {
  statusCb = cb;

  // 1) 저장된 자격증명으로 연결 시도
  String ssid, pass;
  if (loadCredentials(ssid, pass)) {
    if (tryConnect(ssid, pass, connectTimeoutSec)) {
      return true;   // 성공
    }
    Serial.println("[WiFiSetup] Saved credentials failed. Opening portal...");
  } else {
    Serial.println("[WiFiSetup] No saved credentials. Opening portal...");
  }

  // 2) 실패 또는 자격증명 없음 → 설정 포털 진입
  return runConfigPortal(apName, apPassword, portalTimeoutSec);
}
