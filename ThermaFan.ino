#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Helpers from Firebase ESP Client
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Load sensitive credentials
#include "secrets.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Non-blocking timer tracking
unsigned long lastFirebaseUpdate = 0;
const unsigned long FIREBASE_INTERVAL_MS = 5000; // Run every 5 seconds without blocking

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  unsigned long startAttemptTime = millis();
  // Attempt connection with a 15-second non-blocking timeout
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WARN] Wi-Fi connection timed out. Hardware operations will still run!");
  } else {
    Serial.println();
    Serial.print("Connected! IP Address: ");
    Serial.println(WiFi.localIP());
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
  initFirebase();
}

void handleFirebaseTasks() {
  // Execute only if Firebase client is ready and interval has elapsed
  if (Firebase.ready() && (millis() - lastFirebaseUpdate >= FIREBASE_INTERVAL_MS || lastFirebaseUpdate == 0)) {
    lastFirebaseUpdate = millis();

    int testValue = 42;

    // Test write
    Serial.printf("[Firebase] Writing value %d to /test/data...\n", testValue);
    if (Firebase.RTDB.setInt(&fbdo, "/test/data", testValue)) {
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
    if (Firebase.RTDB.getInt(&fbdo, "/test/data")) {
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
