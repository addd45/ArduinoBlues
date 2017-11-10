/*
 * 
 * Info n stuff
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "WifiMcWifiFace";
const char* password = "weakpassword";
const char* mqtt_server = "hussbus.duckdns.org";
const char* mqtt_out_topic = "outTopic";
const char* mqtt_in_topic = "HA/Other/BluesScore";

WiFiClient espClient;
PubSubClient client(espClient);
StaticJsonBuffer<200> jsonBuffer;
long lastReconnectAttempt = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
  lastReconnectAttempt = 5000;
}
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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

bool reconnect(){
  Serial.println("Connecting MQTT...");
  return client.connect("ESP8266Client", "homeassistant", "sb4517");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char message[length];
  
  Serial.println("Message arrived!");

  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  Serial.print("payload: ");
  Serial.println(message);

  JsonObject& rootJson = jsonBuffer.parseObject(message);
  parseJson(rootJson);
}

void mqtt_SendTopic(char* payload){
  // publish an announcement...
  client.publish(mqtt_out_topic, payload);
  // ... and resubscribe
} 

void parseJson(JsonObject& rootJson){
  //Json parse failed?
  if (!rootJson.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  else{
    int blues = rootJson["blues"];
    Serial.println("Parsed Object just fine.");
    Serial.print("blues value: ");
    Serial.print(blues);
  }  
}

void loop()
{
  if (!client.loop()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
    client.subscribe(mqtt_in_topic);  
    Serial.println("Reconnected to mqtt");
      }
    }
  }

}

