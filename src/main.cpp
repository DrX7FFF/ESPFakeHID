#include <Arduino.h>
#include <BleMouse.h>
#include <esp_gap_ble_api.h>

constexpr unsigned long kSendIntervalMs = 120000;
constexpr unsigned long kSendJitterMs   = 60000;
constexpr unsigned long kLoopDelayMs = 50;
constexpr int8_t kMouseDeltaPx = 2;
constexpr unsigned long kMouseStepDelayMs = 20;
BleMouse bleMouse("M240", "Logitech", 100);
unsigned long lastSendAtMs = 0;
unsigned long nextSendIntervalMs = kSendIntervalMs;
bool wasConnected = false;

// Use low-duty-cycle directed advertising toward the bonded Windows host so
// that Windows reconnects automatically on boot (unlike undirected advertising
// which relies on Windows actively scanning at the right moment).
// Falls back to undirected advertising when no bonded device is stored yet.
void startDirectedAdvertising() {
  int count = esp_ble_get_bond_device_num();
  if (count <= 0) {
    Serial.print("No bonded device – undirected advertising\n");
    return;
  }

  count = 1; // Only need the first bonded peer (the Windows host)
  esp_ble_bond_dev_t bondDev;
  esp_ble_get_bond_device_list(&count, &bondDev);

  esp_ble_gap_stop_advertising();
  delay(50);

  esp_ble_adv_params_t params = {};
  params.adv_int_min       = 0x30; // ~30 ms
  params.adv_int_max       = 0x60; // ~60 ms
  params.adv_type          = ADV_TYPE_DIRECT_IND_LOW; // low-duty directed
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
  bleMouse.begin();
  delay(100); // Let BLE stack settle before overriding advertising
  startDirectedAdvertising();
}

void loop() {
  const unsigned long now = millis();
  const bool isConnected = bleMouse.isConnected();

  if (isConnected) {
    if (now - lastSendAtMs >= nextSendIntervalMs) {
      bleMouse.move(-kMouseDeltaPx, 0);
      delay(kMouseStepDelayMs);
      bleMouse.move(kMouseDeltaPx, 0);
      lastSendAtMs = now;
      nextSendIntervalMs = kSendIntervalMs + random(kSendJitterMs + 1);
      Serial.print(".");
    }
  } else if (wasConnected) {
    // Transition: just disconnected – library has restarted undirected adv,
    // override immediately with directed advertising toward the bonded host.
    startDirectedAdvertising();
  }

  wasConnected = isConnected;
  delay(kLoopDelayMs);
}