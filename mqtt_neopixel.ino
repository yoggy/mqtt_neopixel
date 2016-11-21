//
// mqtt_neopixel.ino - sample sketch for WioNode & NeoPixel(WS2812B & WS2811)
//
// how to use:
//
//     $ git clone https://github.com/yoggy/mqtt_neopixel
//     $ cd mqtt_neopixel
//     $ cp config.ino.sample config.ino
//     $ vi config.ino
//       - edit wifi_ssid, wifi_password, mqtt_server, mqtt_subscribe_topic, ... etc
//     $ open mqtt_neopixel.ino
//
// license:
//     Copyright (c) 2016 yoggy <yoggy0@gmail.com>
//     Released under the MIT license
//     http://opensource.org/licenses/mit-license.php;
//

////////////////////////////////////////////////////////////////////////////////////
// for WioNode Pin Assign
#define PORT0_D0    3
#define PORT0_D1    1
#define PORT1_D0    5
#define PORT1_D1    4

#define I2C_SCL     PORT1_D0
#define I2C_SDA     PORT1_D1

#define BUTTON_FUNC 0

#define LED_BLUE    2
#define GROVE_PWR   15

// initialize functions for WioNode.
#define LED_ON()  {digitalWrite(LED_BLUE, LOW);}
#define LED_OFF() {digitalWrite(LED_BLUE, HIGH);}
#define ENABLE_GROVE_CONNECTOR_PWR() {pinMode(GROVE_PWR, OUTPUT);digitalWrite(GROVE_PWR, HIGH);}
#define INIT_WIO_NODE() {pinMode(BUTTON_FUNC, INPUT);pinMode(LED_BLUE, OUTPUT);ENABLE_GROVE_CONNECTOR_PWR();LED_OFF();}

////////////////////////////////////////////////////////////////////////////////////
// for Wifi settings
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/

// Wif config
extern char *wifi_ssid;
extern char *wifi_password;
extern char *mqtt_server;
extern int  mqtt_port;

extern char *mqtt_client_id; 
extern bool mqtt_use_auth;
extern char *mqtt_username;
extern char *mqtt_password;

extern char *mqtt_subscribe_topic;

WiFiClient wifi_client;
void mqtt_sub_callback(char* topic, byte* payload, unsigned int length);
PubSubClient mqtt_client(mqtt_server, mqtt_port, mqtt_sub_callback, wifi_client);

void setup_mqtt() {
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.mode(WIFI_STA);
  int wifi_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (wifi_count % 2 == 0) {
      LED_ON();
    }
    else {
      LED_OFF();
    }
    wifi_count ++;
    delay(500);
  }

  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    reboot();
  }

  LED_ON();
  mqtt_client.subscribe(mqtt_subscribe_topic);
}

void reboot() {
  for (int i = 0; i < 10; ++i) {
    LED_ON()
    delay(100);
    LED_OFF()
    delay(100);
  };

  ESP.restart();

  while (true) {
    LED_ON()
    delay(100);
    LED_OFF()
    delay(100);
  };
}

////////////////////////////////////////////////////////////////////////////////////
// Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel

#define LED_NUM 8
Adafruit_NeoPixel led_strip = Adafruit_NeoPixel(LED_NUM, PORT1_D0, NEO_GRB + NEO_KHZ800); // for WS2812B
// Adafruit_NeoPixel led_strip = Adafruit_NeoPixel(LED_NUM, PORT1_D0, NEO_RGB + NEO_KHZ400); // ws2811

byte *buf = new byte[LED_NUM];

////////////////////////////////////////////////////////////////////////////////////

void setup() {
  INIT_WIO_NODE();
  led_strip.begin();
  setup_mqtt();
}

void loop() {
  if (!mqtt_client.connected()) {
    reboot();
  }
  mqtt_client.loop();
}

void mqtt_sub_callback(char* topic, byte* payload, unsigned int length) {
  LED_OFF()
  delay(50);
  LED_ON()

  parse_command((char*)payload, length);  
}

//
// format:
//     #RRGGBB, #RRGGBB, #RRGGBB, ....
//
// exampple
//     #ff0000, #00ff00, #0000ff, ....
//
#define COMMAND_BUF_LEN 16
void parse_command(char* payload, unsigned int length) {
  char buf[COMMAND_BUF_LEN];
  String color_str;

  int idx = 0;
  char *tp;
  tp = strtok(payload, ",");
  while (tp != NULL) {
    memset(buf, 0, COMMAND_BUF_LEN);

    int len = strlen(tp) < COMMAND_BUF_LEN - 1 ? strlen(tp) : COMMAND_BUF_LEN - 1;
    strncpy(buf, tp, len);

    // trim space & semicolon
    color_str = String(buf);
    color_str.trim();
    if (color_str.substring(color_str.length()-1, color_str.length()) == ",") {
      color_str = color_str.substring(0, color_str.length()-1);
    }

    set_led_color(idx, color_str);

    idx ++;
    if (LED_NUM <= idx) break;

    tp = strtok(NULL, " ");
  }
  led_strip.show();
}

// parse color string (#RRGGBB, ex: #112233->R:0x11, G:0x22, B:0x33)
void set_led_color(int idx, String &color_str) {
  if (color_str.length() != 7) return;

  // check header
  if (color_str.substring(0, 1) != "#") return;

  String str_r = color_str.substring(1, 3);
  String str_g = color_str.substring(3, 5);
  String str_b = color_str.substring(5, 7);

  char *pos;
  uint8_t r = (uint8_t)strtol(str_r.c_str(), &pos, 16);
  uint8_t g = (uint8_t)strtol(str_g.c_str(), &pos, 16 );
  uint8_t b = (uint8_t)strtol(str_b.c_str(), &pos, 16);

  led_strip.setPixelColor(idx, led_strip.Color(r,  g,  b));
}

