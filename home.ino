#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// تنظیمات WiFi
const char* ssid = "Parsa";
const char* password = "Ab12Cd34";

// تنظیمات MQTT
const char* mqtt_server = "87.248.152.17";
const int mqtt_port = 1883;
const char* topic_publish = "relay/status";
const char* topic_subscribe = "relay/control";

WiFiClient espClient;
PubSubClient client(espClient);

// پین‌های  (مطابق با سخت افزار خود تنظیم کنید)
const int relayPins[] = {D4, D3, D2}; // پین‌های رله 1، 2 و 3
const int touchPins[] = {D5, D6, D7}; // پین‌های دکمه لمسی 1، 2 و 3
const int ledPins[] = {D0, RX, TX}; // پین‌های LED 1، 2 و 3

// وضعیت فعلی رله‌ها
bool relayStates[] = {false, false, false};

// زمان آخرین تغییر حالت برای هر رله
unsigned long lastDebounceTime[] = {0, 0, 0};
const long debounceDelay = 200; // تاخیر دبنounce به میلی‌ثانیه

void setup() {
Serial.begin(115200);

// تنظیم پین‌ها
for (int i = 0; i < 3; i++) {
pinMode(relayPins[i], OUTPUT);
pinMode(ledPins[i], OUTPUT);
digitalWrite(relayPins[i], relayStates[i]);
digitalWrite(ledPins[i], !relayStates[i]); // LED وضعیت معکوس رله
}

setup_wifi();
client.setServer(mqtt_server, mqtt_port);
client.setCallback(callback);
}

void setup_wifi() {
delay(10);
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}

Serial.println("");
Serial.println("WiFi connected");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
Serial.print("Message arrived [");
Serial.print(topic);
Serial.print("] ");

String message;
for (int i = 0; i < length; i++) {
message += (char)payload[i];
}
Serial.println(message);

// پردازش پیام MQTT
if (String(topic) == topic_subscribe) {
int relayNum = message.substring(0, 1).toInt();
if (relayNum >= 1 && relayNum <= 3) {
bool newState = message.substring(2) == "ON";
toggleRelay(relayNum - 1, newState);
}
}
}

void reconnect() {
while (!client.connected()) {
Serial.print("Attempting MQTT connection...");
if (client.connect("ESP8266RelayController")) {
Serial.println("connected");
client.subscribe(topic_subscribe);
publishRelayStates();
} else {
Serial.print("failed, rc=");
Serial.print(client.state());
Serial.println(" try again in 5 seconds");
delay(5000);
}
}
}

void toggleRelay(int relayIndex, bool newState) {
if (relayStates[relayIndex] != newState) {
relayStates[relayIndex] = newState;
digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
digitalWrite(ledPins[relayIndex], !relayStates[relayIndex]);
publishRelayStates();
}
}

void publishRelayStates() {
if (client.connected()) {
for (int i = 0; i < 3; i++) {
String message = String(i+1) + ":" + (relayStates[i] ? "ON" : "OFF");
client.publish(topic_publish, message.c_str());
}
}
}

void loop() {
if (!client.connected()) {
reconnect();
}
client.loop();

// بررسی دکمه‌های لمسی
for (int i = 0; i < 3; i++) {
int touchValue = digitalRead(touchPins[i]);

if (touchValue == HIGH && millis() - lastDebounceTime[i] > debounceDelay) {
toggleRelay(i, !relayStates[i]);
lastDebounceTime[i] = millis();
}
}

delay(10);
}
