#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define WIFI_SSID "${wifi_ssid}"
#define WIFI_PASSWORD "${wifi_password}"

#define HOST_NAME "/home/${location}/beam_${id}"
#define LED_STREP_SUBS "/home/${location}/beam_${id}/led_strip"
#define LIGHTING_NAME "/home/${location}/beam_${id}/lighting"

#define OTA_PASSWORD "${ota_password}"

#define MQTT_HOST "${mqtt_host}"
#define MQTT_USER "${mqtt_user}"
#define MQTT_PASSWD "${mqtt_password}"

#define NUM_LEDS 720
#define LED_PIN D2

#define AC_ZERO_CROSS_PIN D0
#define AC_LOAD_PIN D1
#define AC_RELAY_PIN D3

struct Color {
    byte red;
    byte green;
    byte blue;
};

byte algorithmId = 0;
byte ledStrepSpeed = -1;
Color color;

WiFiClient espClient;
PubSubClient client(MQTT_HOST, 1883, espClient);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

byte dimming = 128;

void zero_crosss_int() {
    int dimtime = (75 * dimming);
    delayMicroseconds(dimtime);
    digitalWrite(AC_LOAD_PIN, HIGH);
    delayMicroseconds(8);
    digitalWrite(AC_LOAD_PIN, LOW);
}


void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WIFI ");
    Serial.print(WIFI_SSID);
    Serial.println("...");
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    randomSeed(micros());
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setupDimmer() {
    attachInterrupt(digitalPinToInterrupt(AC_ZERO_CROSS_PIN), zero_crosss_int, RISING);
    pinMode(AC_LOAD_PIN, OUTPUT);
    pinMode(AC_RELE_PIN, OUTPUT);
}

void setupOTA() {
    Serial.println("Starting OTA...");
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(OTA_PASSWORD);

    if (OTA_PASSWORD != "") {
        ArduinoOTA.setPassword((const char *) OTA_PASSWORD);
    }
    ArduinoOTA.begin();
    Serial.println("OTA started");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(MQTT_HOST, MQTT_USER, MQTT_PASSWD)) {
            client.subscribe(LED_STREP_SUBS);
            client.subscribe(LIGHTING_NAME);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    byte argId = 0;
    if (strcmp(topic, LED_STREP_SUBS) == 0) {
        byte algorithmId = -1;
        byte ledStrepSpeed = -1;
        byte brightness = -1;
        Color color;

    } else if (strcmp(topic, LIGHTING_NAME) == 0) {
        if (payload[0] == 0) {
            digitalWrite(AC_RELAY_PIN, LOW);
        } else {
            digitalWrite(AC_RELAY_PIN, HIGH);
        }
        dimming = map(100 - payload[0], 0, 100, 0, 128);
    }
}

void setupMqttClient() {
    client.setCallback(callback);
    reconnect();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    setupDimmer();
    setupWiFi();
    setupOTA();
    setupMqttClient();
}

void loop() {
    ArduinoOTA.handle();
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
