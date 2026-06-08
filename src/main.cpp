#include <Arduino.h>
#include <BleMouse.h>

namespace {
constexpr unsigned long kSendIntervalMs = 120000;
constexpr unsigned long kLoopDelayMs = 50;
constexpr int8_t kMouseDeltaPx = 2;
constexpr unsigned long kMouseStepDelayMs = 20;

BleMouse bleMouse("M705", "Logitech", 100);
unsigned long lastSendAtMs = 0;
}

void setup() {
  Serial.begin(115200);
  Serial.print("Started\n");
  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    const unsigned long now = millis();

    if (lastSendAtMs == 0 || now - lastSendAtMs >= kSendIntervalMs) {
      bleMouse.move(-kMouseDeltaPx, 0);
      delay(kMouseStepDelayMs);
      bleMouse.move(kMouseDeltaPx, 0);
      lastSendAtMs = now;
      Serial.print(".");
    }
  } else {
    lastSendAtMs = 0;
  }

  delay(kLoopDelayMs);
}