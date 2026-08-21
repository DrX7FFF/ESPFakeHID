#include <Arduino.h>
#include <BleMouse.h>
#include <esp_gap_ble_api.h>

constexpr unsigned long kSendIntervalMs = 120000;
constexpr unsigned long kSendJitterMs   = 60000;
constexpr uint8_t kBootButtonPin = 0;
constexpr unsigned long kPairingResetHoldMs = 5000;
constexpr int8_t kMouseDeltaPx = 2;
BleMouse bleMouse("M240", "Logitech", 100);
unsigned long lastSendAtMs = 0;
unsigned long nextSendIntervalMs = kSendIntervalMs;
volatile unsigned long bootPressedAtMs = 0;
volatile bool pairingResetRequested = false;
bool wasConnected = false;

void startDirectedAdvertising();

void IRAM_ATTR onBootButtonChange() {
  if (digitalRead(kBootButtonPin) == LOW) {
    bootPressedAtMs = millis();
  } else if (bootPressedAtMs != 0) {
    if (millis() - bootPressedAtMs > kPairingResetHoldMs) {
      pairingResetRequested = true;
    }
    bootPressedAtMs = 0;
  }
}

void processBootButtonEvent() {
  if (!pairingResetRequested) {
    return;
  }

  int bondCount = esp_ble_get_bond_device_num();
  while (bondCount-- > 0) {
    int oneBond = 1;
    esp_ble_bond_dev_t bondDev;
    if (esp_ble_get_bond_device_list(&oneBond, &bondDev) != ESP_OK || oneBond != 1) {
      Serial.print("Pairing reset failed while reading bond list\n");
      break;
    }
    esp_ble_remove_bond_device(bondDev.bd_addr);
  }

  Serial.print("Pairing list reset, rebooting\n");
  delay(100);
  ESP.restart();
}

// Use low-duty-cycle directed advertising toward the bonded Windows host.
// Falls back to the library's undirected advertising when no bond exists.
void startDirectedAdvertising() {
  int count = esp_ble_get_bond_device_num();
  if (count <= 0) {
    Serial.print("No bonded device - undirected advertising\n");
    return;
  }

  count = 1;
  esp_ble_bond_dev_t bondDev;
  esp_ble_get_bond_device_list(&count, &bondDev);

  esp_ble_gap_stop_advertising();
  delay(50);

  esp_ble_adv_params_t params = {};
  params.adv_int_min       = 0x3000; //40*0.625ms = 30ms // 12288*0.625ms= 7.68s
  params.adv_int_max       = 0x4000; //96*0.625ms = 60ms // 16384*0.625ms= 10.24s
  params.adv_type          = ADV_TYPE_DIRECT_IND_LOW;
  params.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
  memcpy(params.peer_addr, bondDev.bd_addr, sizeof(esp_bd_addr_t));
  params.peer_addr_type    = BLE_ADDR_TYPE_PUBLIC;
  params.channel_map       = ADV_CHNL_ALL;
  params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

  esp_ble_gap_start_advertising(&params);
  Serial.print("Directed advertising started\n");
}

void setup() {
  Serial.begin(115200);
  Serial.print("Started\n");
  pinMode(kBootButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kBootButtonPin), onBootButtonChange, CHANGE);
  bleMouse.begin();
  delay(100); // Let BLE stack settle before overriding advertising
  startDirectedAdvertising();
}

void loop() {
  processBootButtonEvent();
  const bool isConnected = bleMouse.isConnected();

  if (isConnected) {
    if (millis() - lastSendAtMs >= nextSendIntervalMs) {
      bleMouse.move(-kMouseDeltaPx, 0);
      delay(20);
      bleMouse.move(kMouseDeltaPx, 0);
      lastSendAtMs = millis();
      nextSendIntervalMs = kSendIntervalMs + random(kSendJitterMs + 1);
      Serial.print(".");
    }
  } else if (wasConnected) {
    // Transition: just disconnected - library has restarted undirected adv,
    // override immediately with directed advertising toward the bonded host.
    startDirectedAdvertising();
  }

  wasConnected = isConnected;
  delay(500);
}