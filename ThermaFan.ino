#include <WiFi.h>
#include <FirebaseESP32.h>

// Load sensitive credentials
#include "secrets.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Non-blocking timer tracking
unsigned long lastFirebaseUpdate = 0;
const unsigned long FIREBASE_INTERVAL_MS = 5000; // Run every 5 seconds without blocking

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println();
      Serial.println("[Wi-Fi] Status: Connected!");
      Serial.printf("[Wi-Fi] Connected to: %s\n", WiFi.SSID().c_str());
      Serial.printf("[Wi-Fi] IP Address:   %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("[Wi-Fi] Signal (RSSI): %d dBm\n", WiFi.RSSI());
      Serial.println("------------------------------------------");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("[Wi-Fi] Disconnected. Reconnecting...");
      break;
    default:
      break;
  }
}

void initWiFi() {
  Serial.println();
  Serial.println("==========================================");
  Serial.println("   ThermaFan ESP32 System Initializing    ");
  Serial.println("==========================================");

  WiFi.onEvent(onWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi: %s", WIFI_SSID);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[Wi-Fi] Connecting in background...");
  }
}

void syncTime() {
  // SSL needs accurate time for certificate validation
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("[NTP] Syncing clock");
  time_t now = time(nullptr);
  unsigned long startAttempt = millis();
  while (now < 1000000 && millis() - startAttempt < 10000) {
    delay(300);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  if (now > 1000000) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Serial.printf("[NTP] Time synced: %s", asctime(&timeinfo));
  } else {
    Serial.println("[NTP] Time sync pending, continuing...");
  }
}

void initFirebase() {
  // Set Firebase host and legacy token / secret
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // SSL Buffer configurations
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  // Auto-reconnect handling
  config.timeout.wifiReconnect = 10 * 1000;
  Firebase.reconnectNetwork(true);

  // Initialize Firebase client
  Firebase.begin(&config, &auth);
  Firebase.setReadTimeout(fbdo, 1000 * 60);
  Firebase.setwriteSizeLimit(fbdo, "tiny");

  Serial.println("Firebase initialized.");
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Startup settle delay

  initWiFi();
  syncTime();
  initFirebase();
}

void handleFirebaseTasks() {
  // Execute only if Firebase client is ready and interval has elapsed
  if (Firebase.ready() && (millis() - lastFirebaseUpdate >= FIREBASE_INTERVAL_MS || lastFirebaseUpdate == 0)) {
    lastFirebaseUpdate = millis();

    int testValue = 42;

    // Test write
    Serial.printf("[Firebase] Writing value %d to /test/data...\n", testValue);
    if (Firebase.setInt(fbdo, "/test/data", testValue)) {
      Serial.println("[Firebase] Data written successfully!");
      Serial.print("Path: ");
      Serial.println(fbdo.dataPath());
      Serial.print("Type: ");
      Serial.println(fbdo.dataType());
    } else {
      Serial.printf("[Firebase] Write failed: %s\n", fbdo.errorReason().c_str());
    }

    // Test read
    Serial.println("[Firebase] Reading from /test/data...");
    if (Firebase.getInt(fbdo, "/test/data")) {
      Serial.printf("[Firebase] Read value: %d\n", fbdo.intData());
    } else {
      Serial.printf("[Firebase] Read failed: %s\n", fbdo.errorReason().c_str());
    }
  }
}

void loop() {
  // 1. Maintain Firebase non-blocking read/write operations
  handleFirebaseTasks();

  // 2. Add your fan control and sensor readings here (runs smooth and unblocked)
}
