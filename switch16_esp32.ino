#include "FS.h"
#include "SPIFFS.h"
File fsUploadFile;

#define PROJECT_NAME    "Switches 16 Vampire Mine Metz 2021"
#define PROP_NAME    "Switches 16"
#define DEBUG_LED_PIN 2
#define animLedOnTime  300
#define animLedOffTime 100
#define COUNT_OF_BLINK_LED  14
String ssid = "EntS";
String password = "09122019";
uint8_t numberComboAnim = 99;
uint8_t comboCountAnim = 0;
long animTimer = 0;
bool isAnimAllowed = false;


#define CLIENT_ID           52
String CLIENT_NAME = "client_" + String(CLIENT_ID);
const char* mqtt_server = "192.168.0.50";

IPAddress ip(192, 168, 0, CLIENT_ID); //Node static IP
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

#define SWITCHERSCOUNT  16
//uint8_t switcherPin[SWITCHERSCOUNT] = {16,17,18,19,21,22,23,25,26,27,32,33,34,35,36,39};
uint8_t switcherPin[SWITCHERSCOUNT] = {39, 36, 35, 34, 33, 32, 27, 26, 25, 23, 22, 21, 19, 18, 17, 16};
bool isSwitchOn[SWITCHERSCOUNT];

#define COMBOCOUNT      7
#define DIGITSINONECOMBO 4


uint8_t  combo[COMBOCOUNT][DIGITSINONECOMBO] = { {2, 5, 12, 15}, {1, 3, 6, 16}, {2, 7, 9, 13}, {1, 3, 9, 14}, {6, 7, 11, 13}, {7, 10, 14, 16}, {1, 5, 11, 16} };
//  wires          crates        skeletons       Mine plaque     Gauges         Dynamite         Skulls
uint16_t comboValue[COMBOCOUNT];
uint16_t orderWinComboValue[COMBOCOUNT];
bool isComboWin[COMBOCOUNT];
bool isWin = false;
uint16_t switchState = 0;

String _DATE_ = __DATE__;
String _TIME_ = __TIME__;
String _FILE_ = __FILE__;
String _BOARD_ = "arduino";
String msg;
int cntWin;
bool isStateChanged;
//String tmpName = "";
// ----------------------------------------------------- Работа с файловой системой ----------

//--- WEB Server ------------------------------------------------------------------------------
#include <WebServer.h>
WebServer server(80);

#include <WiFiUdp.h>
#include <WiFi.h>
#include <Update.h>
//--------------------------------------------------------------------------- WEB Server -----

#include <DNSServer.h>
DNSServer dnsServer;
const byte DNS_PORT = 53;
int cntWiFiErr = 0, cntMQTTErr = 0;

#include <PubSubClient.h>
const String topic_sub = "/" + CLIENT_NAME + "_sub";
const String topic_pub = "/" + CLIENT_NAME + "_pub";
const String topic_feed = "/" + CLIENT_NAME + "_feed";
const String brdcst_topic = "/broadcast";
bool lastStatusConnectionMQTT = false;
WiFiClient espClient;
PubSubClient client(espClient);
#include "Arduino.h"


#define COUNT_OF_LEDS 7
#define WS2812_PIN    12

long step0 = 0;
long step1 = 0;
long step2 = 0;
long step3 = 0;
long step5 = 0;

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(COUNT_OF_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
bool isButtonPressed = false;
bool itWasLongPress = false;

uint32_t WS2812COLORACTIVE = pixels.Color(255, 255, 255);
uint32_t WS2812COLORBLACK  = pixels.Color(0, 0, 0);

void animWrongCombo(uint8_t comboN)  {
  String mess = "combo_";
  for (int i = 0; i < COMBOCOUNT; i++) {  //get current ID`s timeIndex
    if (orderWinComboValue[i] == switchState)  {
      numberComboAnim = i;
      break;
    }
  }
  comboCountAnim  = 1;
  isStateChanged = false;
  mess += comboN + 1;
  mess += "_already_solved";

  client.publish(topic_pub.c_str(), mess.c_str());
  pixels.setPixelColor(numberComboAnim, WS2812COLORBLACK);
  pixels.show();
  animTimer = millis();
  isAnimAllowed = true;
}

void decodeCombo()  {
  uint16_t value;
  for (int i = 0; i < COMBOCOUNT; i++, value = 0)  {
    for (int x = 0; x < DIGITSINONECOMBO; x++)
      value += (1 << (combo[i][x] - 1));
    comboValue[i] = value;
  }
}

uint16_t checkSwitchers() {
  uint16_t currentSwitchState = 0;
  for (int i = 0; i < SWITCHERSCOUNT; i++) {
    if (!digitalRead(switcherPin[i]))  {
      isSwitchOn[i] = true;
      currentSwitchState |= 1 << i;
    }
    else
      isSwitchOn[i] = false;
  }
  if (currentSwitchState != switchState) {
    client.publish(topic_feed.c_str(), String(currentSwitchState).c_str());
    switchState = currentSwitchState;
    isStateChanged = true;
    //lastComboValue = currentSwitchState;
  }

  return currentSwitchState;
}

void Win()  {
  Serial.println("WIN");
  isWin = true;
  client.publish(topic_pub.c_str(), "solve");
  pixels.fill(WS2812COLORACTIVE);
  pixels.show();

}

void restart()  {
  Serial.println("restart");
  isWin = false;
  isAnimAllowed = false;
  numberComboAnim = 99;
  comboCountAnim = 0;
  for (int i = 0; i < COMBOCOUNT; i++)  {
    isComboWin[i] = false;
    orderWinComboValue[i] = 0;
  }
  pixels.clear();
  pixels.show();
  cntWin = 0;
  decodeCombo();
  client.publish(topic_pub.c_str(), "restart");
}

void wifi_status () {
  Serial.print("  WiFi status=");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WL_CONNECTED");
  }
  else if (WiFi.status() == WL_NO_SHIELD) {
    Serial.print("WL_NO_SHIELD");
  }
  else if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.print("WL_IDLE_STATUS");
  }
  else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.print("WL_NO_SSID_AVAIL");
  }
  else if (WiFi.status() == WL_SCAN_COMPLETED) {
    Serial.print("WL_SCAN_COMPLETED");
  }
  else if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.print("WL_CONNECT_FAILED");
  }
  else if (WiFi.status() == WL_CONNECTION_LOST) {
    Serial.print("WL_CONNECTION_LOST");
  }
  else if (WiFi.status() == WL_DISCONNECTED) {
    Serial.print("WL_DISCONNECTED");
  }
}

void setup_wifi(bool reboot) {
  WiFi.mode(WIFI_OFF);
  delay(2000);
  WiFi.mode(WIFI_STA);

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long tmp = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    delay(200);
    Serial.println("");
    wifi_status ();
    //Serial.print(".");
    if (millis() > tmp + 30000) {
      if (reboot) {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        Serial.println("\n\nESP wait 20sec and will be reset()");
        delay(20000);
        Serial.println("\n\nESP will be reset because 60sec not connecting..");
        delay(200);
#ifdef ESP32
        ESP.restart();
#endif
#ifdef ESP8266
        ESP.reset();
#endif
      }
      else {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        Serial.println("\n\nESP wait 10sec ");
        delay(10000);
        break;
      }
    }
  }
  randomSeed(micros());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi ");
    Serial.print(ssid);
    Serial.println(" connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    reconnect();
  }
  else {
    Serial.println("");
    Serial.print("WiFi ");
    Serial.print(ssid);
    Serial.println(" connection FAIL");
  }
  FS_init();
  webServer_init();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String _msg = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    _msg += (char)payload[i];
  }
  Serial.println();
  if (_msg == "restart") {
    restart();
  }
  if (_msg == "solve") {
    Win();
  }
  if ((char)payload[0] == '3') {
    client.publish(topic_pub.c_str(), "31");
  }
  else if ((char)payload[0] == '9') {
    ESP.restart();
  }
}

void reconnect() {
  Serial.println("Connecting to MQTT..");
  if (client.connect(CLIENT_NAME.c_str())) {
    Serial.println("Connected");
    client.publish(topic_pub.c_str(), "Connected");
    client.subscribe(topic_sub.c_str());
    digitalWrite(DEBUG_LED_PIN, LOW);
  }
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    cntMQTTErr++;
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(100);
    digitalWrite(DEBUG_LED_PIN, LOW);
    delay(50);
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(100);
    digitalWrite(DEBUG_LED_PIN, LOW);
    delay(50);
    digitalWrite(DEBUG_LED_PIN, HIGH);
    delay(100);
    digitalWrite(DEBUG_LED_PIN, LOW);
    delay(50);
  }
}

void setup() {
#ifdef ESP32
  Serial.begin(115200);
  Serial.println("ESP32");
  _BOARD_ = "ESP32";
  pinMode(DEBUG_LED_PIN, OUTPUT);
  digitalWrite(DEBUG_LED_PIN, HIGH);
#endif
#ifdef ESP8266
  Serial.begin(74880);
  Serial.println("ESP8266");
  _BOARD_ = "ESP8266";
  pinMode(DEBUG_LED_PIN, OUTPUT);
  digitalWrite(DEBUG_LED_PIN, LOW);
#endif

  Serial.print("\n\n  starting ");
  Serial.println(PROJECT_NAME);
  Serial.print(_DATE_);
  Serial.print("  ");
  Serial.println(_TIME_);
  Serial.println("File -> " + _FILE_ );
  for (int i = 0; i < SWITCHERSCOUNT; i++)
    pinMode(switcherPin[i], INPUT_PULLUP);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  setup_wifi(true);

  reconnect();
  delay(200);
  restart();
  Serial.println("____________________________");

}

void comboWin(int comboN) {
  String cmbWin = "comboWin_";
  Serial.print("\ncombo ");
  Serial.print(comboN);
  orderWinComboValue[cntWin] = switchState;
  cntWin++;
  cmbWin += cntWin;
  Serial.println(" win");
  isComboWin[comboN] = true;
  isStateChanged = false;
  client.publish(topic_pub.c_str(), cmbWin.c_str());
  pixels.setPixelColor(cntWin - 1, WS2812COLORACTIVE);
  pixels.show();
  if ((cntWin == COMBOCOUNT) && (!isWin))
    Win();

  switch (comboN) {
    case 0:
      client.publish(topic_pub.c_str(), "wires");
      break;

    case 1:
      client.publish(topic_pub.c_str(), "crates");
      break;

    case 2:
      client.publish(topic_pub.c_str(), "skeletons");
      break;

    case 3:
      client.publish(topic_pub.c_str(), "plaque");
      break;

    case 4:
      client.publish(topic_pub.c_str(), "gauges");
      break;

    case 5:
      client.publish(topic_pub.c_str(), "dynamite");
      break;

    case 6:
      client.publish(topic_pub.c_str(), "skulls");
      break;

    default:
      client.publish(topic_pub.c_str(), "wrong comb data");
      break;
  }

}

void loop() {
  
  if (isAnimAllowed && (millis() - animTimer > ((comboCountAnim % 2) ? animLedOnTime : animLedOffTime))) {
    animTimer = millis();
    pixels.setPixelColor(numberComboAnim, (comboCountAnim % 2) ? WS2812COLORACTIVE : WS2812COLORBLACK);
    if (++comboCountAnim == COUNT_OF_BLINK_LED)  {
      isAnimAllowed = false;
      pixels.setPixelColor(numberComboAnim,  WS2812COLORACTIVE);
    }
    pixels.show();
  }

  if (millis() - step0 > 1000) {
    step0 = millis();
#ifdef ESP32
    digitalWrite (DEBUG_LED_PIN, HIGH);
    delay(5);
    digitalWrite (DEBUG_LED_PIN, LOW);
#endif
#ifdef ESP8266
    digitalWrite (DEBUG_LED_PIN, LOW);
    delay(5);
    digitalWrite (DEBUG_LED_PIN, HIGH);
#endif
    Serial.println("");
    Serial.print(PROP_NAME);
    Serial.print(". IP = ");
    Serial.print(WiFi.localIP());
    Serial.print(". freeHeap = ");
    Serial.print(ESP.getFreeHeap());
    wifi_status ();
    Serial.print("  upTime=");
    Serial.print(millis() / 1000.);
    Serial.print(". Signal = ");
    Serial.println(WiFi.RSSI());
  }

  if (lastStatusConnectionMQTT != client.connected()) {
    lastStatusConnectionMQTT = client.connected();
    reconnect();
  } else if (!client.connected() and (millis() - step1) > 45000) {
    step1 = millis();
    reconnect();
  }

  if (millis() - step2 > 50) {
    step2 = millis();
    checkSwitchers();
    int x = 0;
    for (int i = 0; i < COMBOCOUNT; i++) {
      if (switchState == comboValue[i])  {
        if (!isComboWin[i])
          comboWin(i);
        /*//*/ else if (isStateChanged) {
          //else if(isComboWin[i] && isStateChanged)  {
          animWrongCombo(i);
        }
      }
    }
  }

  if ( millis() - step5 >= 60000 ) {
    if ((WiFi.status() != WL_CONNECTED)) {
      Serial.print(millis());
      Serial.println("\n\n Reconnecting to WiFi...");

      setup_wifi(false);
      cntWiFiErr ++;
    }
    step5 = millis();
  }

  server.handleClient();
  yield();
  client.loop();
  yield();
}
