#include "sim.hpp"

enum lcd_state {State1, State2, State3, ResetState};
lcd_state current_lcd_state;
lcd_state next_lcd_state;
long suma1 = 0;
long suma2 = 0;
int last_read;
int read_pin = 0;
long pulse_cnt = 0;

void PrintVrednost(long vrednost){
    comb_print("                ", 0, 1);
    Serial.print("Ucitana vrednost: ");
    comb_print(String(vrednost), 0, 1);
}

bool rising_edge(int last_read, int read){
  if (last_read == 0 && read == 1){
      return true;
  }
  //Serial.println("PA TO JE SJAJNO");
  //Serial.print(last_read);
  //Serial.print(read);
  //delay(4000);
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(RPIN, INPUT);

  nav_button.setDebounceTime(BUTTON_DEBOUNCE_TIME);
  reset_button.setDebounceTime(BUTTON_DEBOUNCE_TIME);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  comb_print("PiLab 021", 0, 0);
  delay(1000);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connect_to_wifi));

  WiFi.onEvent(WiFiEvent);

  mqttclient.onConnect(onMqttConnect);
  mqttclient.onDisconnect(onMqttDisconnect);
  mqttclient.onPublish(onMqttPublish);
  mqttclient.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  connect_to_wifi();

  current_lcd_state = ResetState;
  next_lcd_state = State1;

  randomSeed(analogRead(0));
}

void loop() {
  nav_button.loop();
  reset_button.loop();

  current_time = millis();
  long rndNmbr = random(100);
  last_read = read_pin;
  read_pin = digitalRead(RPIN);

  if(current_lcd_state != next_lcd_state){
    current_lcd_state = next_lcd_state;
    lcd.clear();
    switch (current_lcd_state){
        case State1: 
          Serial.print("Prikaz protoka");
          comb_print("Protok vode: ", 0, 0);
          PrintVrednost(pulse_cnt);
          break;
        case State2:
          Serial.print("Prikaz ukupnog protoka");
          comb_print("Ukupan protok1: ", 0, 0);
          PrintVrednost(suma1);
          break;
        case State3:
          Serial.print("Prikaz ukupnog protoka (ne-resetivog)");
          comb_print("Ukupan protok2: ", 0, 0);
          PrintVrednost(suma2);
          break;
        case ResetState:
          Serial.print("Zahtev za reset ukupnog protoka");
          comb_print("Potvrdite reset", 0, 0);
          break;
        default:
          Serial.print("Prikaz protoka");
          comb_print("Protok vode: ", 0, 0);
          PrintVrednost(pulse_cnt);
          break;
      }
  }

  if(rising_edge(last_read, read_pin)){
    pulse_cnt++;
  }

  if(nav_button.isPressed()){
    switch (current_lcd_state){
      case State1: next_lcd_state = State2; break;
      case State2: next_lcd_state = State3; break; 
      case State3: next_lcd_state = State1; break;
      default: next_lcd_state = State1; break;
    }   
  }

  if(reset_button.isPressed()){
    if(current_lcd_state != ResetState){
      next_lcd_state = ResetState;
      reset_timer = millis();
      reset_pressed = true;
    }else{
      suma1 = 0;
      comb_print("Ukupni protok je", 0, 0);
      comb_print("resetovan", 0, 1);
      delay(1000);
      next_lcd_state = State1;
    }
  }

  if((current_time - reset_timer > 7000) && reset_pressed){
    next_lcd_state = State1;
    reset_pressed = false;
  }

  if((current_time - last_measure > 5000) || (last_measure == 0)){
    last_measure = current_time;
    suma1 += rndNmbr;
    suma2 += rndNmbr;

    switch (current_lcd_state){
          case State1: PrintVrednost(pulse_cnt); break;
          case State2: PrintVrednost(suma1); break; 
          case State3: PrintVrednost(suma2); break;
          default: break;
    }

    // MQTT Publish 
    uint16_t packetIdPub1 = mqttclient.publish(MQTT_TOPIC_1, 1, true, String(pulse_cnt).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_TOPIC_1, packetIdPub1);
    Serial.printf("Message: %ld \n", pulse_cnt);

    uint16_t packetIdPub2 = mqttclient.publish(MQTT_TOPIC_2, 1, true, String(suma1).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_TOPIC_2, packetIdPub2);
    Serial.printf("Message: %ld \n", suma1);

    uint16_t packetIdPub3 = mqttclient.publish(MQTT_TOPIC_3, 1, true, String(suma2).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_TOPIC_3, packetIdPub3);
    Serial.printf("Message: %ld \n", suma2);
  }
}