#include <Arduino.h>
#include <BleKeyboard.h>

namespace {
constexpr unsigned long kSendIntervalMs = 120000;
constexpr unsigned long kLoopDelayMs = 50;

BleKeyboard bleKeyboard("K400 Plus", "Logitech", 100);
unsigned long lastSendAtMs = 0;
}

void setup() {
  Serial.begin(115200);
  Serial.print("Started\n");
  bleKeyboard.begin();
}

void loop() {
  if (bleKeyboard.isConnected()) {
    const unsigned long now = millis();

    if (lastSendAtMs == 0 || now - lastSendAtMs >= kSendIntervalMs) {
      // bleKeyboard.write(KEY_F15);
      bleKeyboard.press(KEY_LEFT_SHIFT);
      delay(100);
      bleKeyboard.release(KEY_LEFT_SHIFT);
      lastSendAtMs = now;
      Serial.print(".");
    }
  } else {
    lastSendAtMs = 0;
  }

  delay(kLoopDelayMs);
}