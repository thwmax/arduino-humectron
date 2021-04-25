#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define SERVER_IP "IP_ADDRESS"

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#define RX_PORT 5
#define TX_PORT 4
#endif

SoftwareSerial mySerial(RX_PORT, TX_PORT); // (Rx, Tx)
StaticJsonDocument<2048> jsonBuffer;

void setup() {
  // initialize serial:
  Serial.begin(115200);
  mySerial.begin(38400);
  
  while (!Serial) continue;

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  mySerial.setTimeout(3000);
}

void doPostRequest(String json, String url) {
  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin(client, url); //HTTP
    http.addHeader("Content-Type", "application/json");

    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    int httpCode = http.POST(json);

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void sendTemperature(float value) {
  String url = "http://" SERVER_IP ":3000/temperatures";
  
  DynamicJsonDocument doc(2048);
  doc["value"] = value;
  doc["unit"] = "c";
  doc["source"] = "Arduino Wemos D1";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  doPostRequest(json, url);
}

void sendMoisture(float value) {
  String url = "http://" SERVER_IP ":3000/moisture-readings";
  
  DynamicJsonDocument doc(2048);
  doc["value"] = value;
  doc["source"] = "Arduino Wemos D1";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  doPostRequest(json, url);
}

void loop() {
  if (mySerial.available() > 0) {
    Serial.print("Data available!\n");
    delay(10);
    String s = mySerial.readStringUntil('#');

    while (mySerial.available() > 0) mySerial.read();
    Serial.print("Raw json ");
    Serial.print(s);
    Serial.print("\n");
    
    DeserializationError error = deserializeJson(jsonBuffer, s);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    Serial.println("JSON received");
    JsonObject root = jsonBuffer.as<JsonObject>();
    serializeJson(root, Serial);
    sendTemperature(root["temp"]);
    sendMoisture(root["moisture"]);
    Serial.println("---------------------xxxxx--------------------");
  }
}