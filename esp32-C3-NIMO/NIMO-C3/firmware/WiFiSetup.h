// ==================================================
// WiFiSetup.h
// ESP32-C3 + Core 3.x 호환 자체구현 WiFi 설정 매니저
// 외부 라이브러리 의존성 없음 (Preferences + WebServer + DNSServer 내장 사용)
// ==================================================
#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>

// 화면 콜백 타입: 진행 상황을 OLED에 표시하기 위해 메인 코드에서 함수를 넘겨줌
// (line1, line2 두 줄짜리 메시지를 표시)
typedef void (*WiFiStatusCallback)(const char* line1, const char* line2);

// WiFi 연결 시도 + 실패 시 설정 포털(AP) 진입까지 처리하는 메인 함수
//   apName     : 설정 AP 이름 (예: "NIMO-Setup")
//   apPassword : 설정 AP 비밀번호 (8자 이상, nullptr이면 오픈 AP)
//   connectTimeoutSec : 저장된 WiFi 연결 시도 제한시간(초)
//   portalTimeoutSec  : 설정 포털 대기 제한시간(초). 이 시간 지나면 재부팅
//   cb         : OLED 표시용 콜백 (nullptr 가능)
// 반환값: true = WiFi 연결 성공, false = 실패
bool wifiSetupBegin(const char* apName,
                    const char* apPassword,
                    uint32_t connectTimeoutSec,
                    uint32_t portalTimeoutSec,
                    WiFiStatusCallback cb);

// 저장된 WiFi 자격증명 삭제 (버튼 길게 누르기 등으로 호출)
void wifiSetupReset();

// 저장된 SSID가 있는지 확인
bool wifiSetupHasCredentials();

#endif // WIFI_SETUP_H
