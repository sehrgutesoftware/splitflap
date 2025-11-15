/*
   Copyright 2025 Scott Bezek and the splitflap contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "network_config.h"

#ifdef USE_ETHERNET
static bool eth_connected = false;

void NetworkConfig::onEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("splitflap");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void NetworkConfig::begin(const char* ssid, const char* password) {
  WiFi.onEvent(onEvent);
  // WT32-ETH01 pinout
  ETH.begin(1, 16, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_IN);
}

bool NetworkConfig::isConnected() {
  return eth_connected;
}

String NetworkConfig::localIP() {
  return ETH.localIP().toString();
}

void NetworkConfig::setSleep(bool enable) {
  // Ethernet doesn't support sleep mode
}

#else // WiFi mode

void NetworkConfig::begin(const char* ssid, const char* password) {
  if (ssid != nullptr && password != nullptr) {
    WiFi.begin(ssid, password);
  }
}

bool NetworkConfig::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkConfig::localIP() {
  return WiFi.localIP().toString();
}

void NetworkConfig::setSleep(bool enable) {
  WiFi.setSleep(enable ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
}

#endif
