#include "sim.hpp"

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
AsyncMqttClient mqttclient;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

ezButton nav_button(BUTTON_1);
ezButton reset_button(BUTTON_2);

long current_time = 0;
long last_measure = 0;
long reset_timer = 0;
bool reset_pressed = false;

void connect_to_wifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(SSID, PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttclient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0);
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  Serial.println(static_cast<uint8_t>(reason));
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void comb_print(String text, short cursor, short row){
  Serial.println(text);
  lcd.setCursor(cursor, row);
  lcd.print(text);
}