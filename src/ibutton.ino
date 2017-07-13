#include <OneWire.h>
#include <Homie.h>

#define IBUTTON_PIN 5
#define LED_PIN 4
//#define LED_PIN LED_BUILTIN

#define IBUTTON_READING_INTERVAL 250 /* ms */

OneWire onewire(IBUTTON_PIN);
int brightness = PWMRANGE;
bool led_on = false;
unsigned long ibutton_reading_interval_start = 0;

HomieNode ledNode("led", "switch");
HomieNode ibuttonNode("ibutton", "sensor");

void updateLed() {
  if (brightness == 0) {
    led_on = false;
  }

  if (led_on) {
    analogWrite(LED_PIN, brightness);
  } else {
    analogWrite(LED_PIN, 0);
  }

  ledNode.setProperty("brightness").send(String(brightness));
  ledNode.setProperty("status").send(led_on ? "ON" : "OFF");
}

bool ledBrightnessHandler(const HomieRange& range, const String& value) {
  brightness = value.toInt();
  if (brightness > PWMRANGE) { brightness = PWMRANGE; }
  if (brightness < 0) { brightness = 0; }

  updateLed();
  return true;
}

bool ledStatusHandler(const HomieRange& range, const String& value) {
  if ((value != "ON") && (value != "OFF")) return false;

  if (value == "ON") {
    led_on = true;
  } else {
    led_on = false;
  }

  updateLed();
  return true;
}

void iButtonLoopHandler() {

  byte addr[8];
  static byte last_crc = 0;
  static bool devices_found = false;

  if (!onewire.search(addr)) {
    onewire.reset_search();
    if (devices_found == false) {        
        if (last_crc != 0) {
           // No devices connected
           ibuttonNode.setProperty("address").send("00:00:00:00:00:00:00:00");
           last_crc = 0;
        }
    }

    devices_found = false;
    return;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    // Address not read properly
    return;
  }

  devices_found = true;

  //convert addr to string
  char addr_str[8*3+1];
  int pos;
  for (int i=0; i<=7; i++) {
    pos=i*3;
    snprintf((char*)(&addr_str)+pos, sizeof(addr_str)-pos,"%02X:", addr[i]);
  }
  addr_str[7*3+2]=0;

  if (last_crc != addr[7]) {
    ibuttonNode.setProperty("address").send(addr_str);
  }
  last_crc = addr[7];

}

void setupHandler() {
  ledNode.setProperty("brightness").send(String(brightness));
  ledNode.setProperty("status").send("OFF");
}

void loopHandler() {
  unsigned long now = millis();

  if ((now - ibutton_reading_interval_start) >= IBUTTON_READING_INTERVAL) {
    iButtonLoopHandler();
    ibutton_reading_interval_start = now;
  }

}

void setup(void) {
  Serial.begin(115200);
  Serial.println("\n[iButton WiFi Reader]\n");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Homie_setFirmware("iButton-reader", "0.0.1")
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

  ledNode.advertise("brightness").settable(ledBrightnessHandler);
  ledNode.advertise("status").settable(ledStatusHandler);

  Homie.setup();
}

void loop(void) {

  Homie.loop();
  /*
  byte addr[8];

  if (!onewire.search(addr)) {
    onewire.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");
  for (int i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }
  Serial.println();

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid !");
    return;
  }

  toggle_led();
  */
}
