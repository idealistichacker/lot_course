# 1 "d:\\DevCode\\lot\\esp32_firmware\\main.ino"
# 2 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 2
# 3 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 2
# 4 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 2

// Optional camera support for ESP32-S3-CAM.
// Set to 1 after you verify camera pin mapping and library availability.






# 14 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 2
# 27 "d:\\DevCode\\lot\\esp32_firmware\\main.ino"
DHT dht(2, DHT11);
__attribute__((section(".rtc.data" "." "0"))) uint32_t bootCount = 0;
__attribute__((section(".rtc.data" "." "1"))) uint32_t sequenceId = 0;

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
  Serial0.println("Scanning nearby WiFi...");
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    Serial0.println("No WiFi found.");
    return;
  }

  Serial0.printf("Found %d networks:\n", n);
  for (int i = 0; i < n && i < 10; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    uint8_t enc = WiFi.encryptionType(i);
    Serial0.printf("  %d) SSID=%s RSSI=%d ENC=%u\n", i + 1, ssid.c_str(), rssi, enc);
  }
}

bool connectWiFi() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setSleep(false);
  scanNearbyNetworks();
  Serial0.printf("Connecting SSID='%s' ...\n", "esp32");
  WiFi.begin("esp32", "yuyangsuai");

  uint32_t start = millis();
  wl_status_t lastStatus = WL_IDLE_STATUS;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    wl_status_t cur = WiFi.status();
    if (cur != lastStatus) {
      Serial0.printf("WiFi status => %s (%d)\n", wifiStatusText(cur), (int)cur);
      lastStatus = cur;
    }
    delay(500);
    Serial0.print(".");
  }
  Serial0.println();

  wl_status_t finalStatus = WiFi.status();
  if (finalStatus == WL_CONNECTED) {
    Serial0.printf("WiFi connected. IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }

  Serial0.printf("WiFi connect timeout. final_status=%s (%d)\n", wifiStatusText(finalStatus), (int)finalStatus);
  return false;
}

String makeJsonBody(const SensorPacket &pkt) {
  char buf[512];
  snprintf(
      buf,
      sizeof(buf),
      "{\"device_id\":\"%s\",\"fw_version\":\"%s\",\"timestamp_epoch\":0,\"seq\":%lu,\"boot_count\":%lu,\"uptime_ms\":%llu,\"temp_c\":%.2f,\"rh_pct\":%.2f,\"dew_point_c\":%.2f,\"wifi_rssi\":%d,\"sensor_ok\":%s}",
      "esp32s3-cam-01",
      "0.1.0",
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
  for (int i = 1; i <= 3; ++i) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial0.println("WiFi lost before POST, reconnecting...");
      if (!connectWiFi()) {
        delay(2000);
        continue;
      }
    }

    HTTPClient http;
    http.setTimeout(8000);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Id", "esp32s3-cam-01");

    httpCode = http.POST(payload);
    respBody = http.getString();
    http.end();

    Serial0.printf("POST try %d/%d, code=%d\n", i, 3, httpCode);
    if (httpCode > 0 && httpCode < 300) {
      return true;
    }

    delay(2000 * i);
  }
  return false;
}
# 216 "d:\\DevCode\\lot\\esp32_firmware\\main.ino"
void enterDeepSleep() {
  const uint64_t sleepUs = (uint64_t)1 * 60ULL * 1000000ULL;
  Serial0.printf("Entering deep sleep for %d min...\n", 1);
  esp_sleep_enable_timer_wakeup(sleepUs);
  Serial0.flush();
  esp_deep_sleep_start();
}

void setup() {
  Serial0.begin(115200);
  delay(800);

  bootCount++;
  sequenceId++;
  Serial0.printf("Boot #%lu, seq=%lu\n", (unsigned long)bootCount, (unsigned long)sequenceId);

  dht.begin();





  float tempC = 
# 238 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 3 4
               (__builtin_nanf(""))
# 238 "d:\\DevCode\\lot\\esp32_firmware\\main.ino"
                  ;
  float rh = 
# 239 "d:\\DevCode\\lot\\esp32_firmware\\main.ino" 3 4
            (__builtin_nanf(""))
# 239 "d:\\DevCode\\lot\\esp32_firmware\\main.ino"
               ;
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
    Serial0.printf("DHT11 OK: T=%.2fC RH=%.2f%% DP=%.2fC\n", pkt.tempC, pkt.rh, pkt.dewPoint);
  } else {
    Serial0.println("DHT11 read failed after retries.");
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
    sent = postJsonWithRetry("http://192.168.34.48:8000/upload", body, httpCode, resp);
  }

  if (sent) {
    Serial0.printf("Upload success. code=%d body=%s\n", httpCode, resp.c_str());
  } else {
    Serial0.printf("Upload failed. code=%d body=%s\n", httpCode, resp.c_str());
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_MODE_NULL);

  enterDeepSleep();
}

void loop() {
  // Unused. Device works in wake-do-sleep mode.
}
