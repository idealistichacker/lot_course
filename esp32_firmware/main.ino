#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Optional camera support for ESP32-S3-CAM.
// Set to 1 after you verify camera pin mapping and library availability.
#define ENABLE_CAMERA 0

#if ENABLE_CAMERA
#include "esp_camera.h"
#endif

#include "secrets.h"

#define DHT_PIN 2
#define DHT_TYPE DHT11

#define HTTP_TIMEOUT_MS 8000
#define MAX_RETRY 3
#define RETRY_BACKOFF_MS 2000
#define WIFI_CONNECT_TIMEOUT_MS 20000

#ifndef WAKE_INTERVAL_MIN
#define WAKE_INTERVAL_MIN 15
#endif

DHT dht(DHT_PIN, DHT_TYPE);
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint32_t sequenceId = 0;

struct SensorPacket {
  bool ok;
  float tempC;
  float rh;
  float dewPoint;
  int wifiRssi;
  uint32_t seq;
  uint32_t boot;
  uint64_t uptimeMs;
};

float calcDewPoint(float tempC, float rh) {
  // Project approximation: Tdp ~= T - (100 - RH) / 5
  return tempC - ((100.0f - rh) / 5.0f);
}

bool readDHTWithRetry(float &tempC, float &rh) {
  for (int i = 0; i < 5; ++i) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h) && h >= 0 && h <= 100) {
      tempC = t;
      rh = h;
      return true;
    }
    delay(1500);
  }
  return false;
}

const char *wifiStatusText(wl_status_t s) {
  switch (s) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void scanNearbyNetworks() {
  Serial.println("Scanning nearby WiFi...");
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    Serial.println("No WiFi found.");
    return;
  }

  Serial.printf("Found %d networks:\n", n);
  for (int i = 0; i < n && i < 10; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    uint8_t enc = WiFi.encryptionType(i);
    Serial.printf("  %d) SSID=%s RSSI=%d ENC=%u\n", i + 1, ssid.c_str(), rssi, enc);
  }
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  scanNearbyNetworks();
  Serial.printf("Connecting SSID='%s' ...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  wl_status_t lastStatus = WL_IDLE_STATUS;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    wl_status_t cur = WiFi.status();
    if (cur != lastStatus) {
      Serial.printf("WiFi status => %s (%d)\n", wifiStatusText(cur), (int)cur);
      lastStatus = cur;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  wl_status_t finalStatus = WiFi.status();
  if (finalStatus == WL_CONNECTED) {
    Serial.printf("WiFi connected. IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }

  Serial.printf("WiFi connect timeout. final_status=%s (%d)\n", wifiStatusText(finalStatus), (int)finalStatus);
  return false;
}

String makeJsonBody(const SensorPacket &pkt) {
  char buf[512];
  snprintf(
      buf,
      sizeof(buf),
      "{\"device_id\":\"%s\",\"fw_version\":\"%s\",\"timestamp_epoch\":0,\"seq\":%lu,\"boot_count\":%lu,\"uptime_ms\":%llu,\"temp_c\":%.2f,\"rh_pct\":%.2f,\"dew_point_c\":%.2f,\"wifi_rssi\":%d,\"sensor_ok\":%s}",
      DEVICE_ID,
      FW_VERSION,
      (unsigned long)pkt.seq,
      (unsigned long)pkt.boot,
      (unsigned long long)pkt.uptimeMs,
      pkt.tempC,
      pkt.rh,
      pkt.dewPoint,
      pkt.wifiRssi,
      pkt.ok ? "true" : "false");

  return String(buf);
}

bool postJsonWithRetry(const String &url, const String &payload, int &httpCode, String &respBody) {
  for (int i = 1; i <= MAX_RETRY; ++i) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost before POST, reconnecting...");
      if (!connectWiFi()) {
        delay(RETRY_BACKOFF_MS);
        continue;
      }
    }

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Id", DEVICE_ID);

    httpCode = http.POST(payload);
    respBody = http.getString();
    http.end();

    Serial.printf("POST try %d/%d, code=%d\n", i, MAX_RETRY, httpCode);
    if (httpCode > 0 && httpCode < 300) {
      return true;
    }

    delay(RETRY_BACKOFF_MS * i);
  }
  return false;
}

#if ENABLE_CAMERA
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 18;
  config.pin_d6 = 17;
  config.pin_d7 = 16;
  config.pin_xclk = 15;
  config.pin_pclk = 13;
  config.pin_vsync = 6;
  config.pin_href = 7;
  config.pin_sscb_sda = 4;
  config.pin_sscb_scl = 5;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  Serial.println("Camera init OK.");
  return true;
}
#endif

void enterDeepSleep() {
  const uint64_t sleepUs = (uint64_t)WAKE_INTERVAL_MIN * 60ULL * 1000000ULL;
  Serial.printf("Entering deep sleep for %d min...\n", WAKE_INTERVAL_MIN);
  esp_sleep_enable_timer_wakeup(sleepUs);
  Serial.flush();
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(800);

  bootCount++;
  sequenceId++;
  Serial.printf("Boot #%lu, seq=%lu\n", (unsigned long)bootCount, (unsigned long)sequenceId);

  dht.begin();

#if ENABLE_CAMERA
  initCamera();
#endif

  float tempC = NAN;
  float rh = NAN;
  bool sensorOk = readDHTWithRetry(tempC, rh);

  SensorPacket pkt;
  pkt.ok = sensorOk;
  pkt.tempC = sensorOk ? tempC : -999.0f;
  pkt.rh = sensorOk ? rh : -1.0f;
  pkt.dewPoint = sensorOk ? calcDewPoint(tempC, rh) : -999.0f;
  pkt.wifiRssi = -127;
  pkt.seq = sequenceId;
  pkt.boot = bootCount;
  pkt.uptimeMs = millis();

  if (sensorOk) {
    Serial.printf("DHT11 OK: T=%.2fC RH=%.2f%% DP=%.2fC\n", pkt.tempC, pkt.rh, pkt.dewPoint);
  } else {
    Serial.println("DHT11 read failed after retries.");
  }

  bool wifiOk = connectWiFi();
  if (wifiOk) {
    pkt.wifiRssi = WiFi.RSSI();
  }

  String body = makeJsonBody(pkt);
  int httpCode = -1;
  String resp;

  bool sent = false;
  if (wifiOk) {
    sent = postJsonWithRetry(UPLOAD_URL, body, httpCode, resp);
  }

  if (sent) {
    Serial.printf("Upload success. code=%d body=%s\n", httpCode, resp.c_str());
  } else {
    Serial.printf("Upload failed. code=%d body=%s\n", httpCode, resp.c_str());
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  enterDeepSleep();
}

void loop() {
  // Unused. Device works in wake-do-sleep mode.
}
