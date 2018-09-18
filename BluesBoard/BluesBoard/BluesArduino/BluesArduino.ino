
/*
 * 
 * Info n stuff
 */

#include <ArduinoJson.hpp>
#include "Enums.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <ArduinoJson.h>

#define BIG_DISPLAY_ADDRESS 0x70
#define CS_PIN 10

const char* ssid = "WifiMcWifiFace";
const char* password = "weakpassword";
const char* mqtt_server = "hussbus.duckdns.org";
const char* mqtt_out_topic = "outTopic";
const char* mqtt_in_topic = "Other/BluesScore";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_7segment scoreDisplay = Adafruit_7segment();
DisplayType displayTypes;
long lastReconnectAttempt = 0;

void setup() {
  //Hardware setup
  Serial.begin(115200);
  setup_wifi();
  scoreDisplay.begin(BIG_DISPLAY_ADDRESS);
  scoreDisplay.print(9999, DEC);
  scoreDisplay.writeDisplay();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); //By default, don't be selecting OpenSegment
  SPI.begin(); //Start the SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV64); //Slow down the master a bit
  digitalWrite(CS_PIN, LOW); //Drive the CS pin low to select OpenSegment
  SPI.transfer('v'); //Reset command

  //MQTT 
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
  char message[128];
  BoxScore boxScore;
  
  Serial.println("Message arrived!");
  for (short cnt = 0; cnt < length; cnt++)
  {
	  // convert byte to its ascii representation
	  message[cnt] = (char)payload[cnt];
  }

  Serial.print("payload: ");
  // display
  Serial.println(message);

  boxScore = ParseMessage(message);
  NewBoxScore(boxScore);
  
  Serial.println("Displays Updated");
}


void UpdateDisplay(short blues, short opp, DisplayType display) {
	/*
	  Scoreboard assumes Blues always the left 2 segments
	  And away team right 2 segments
	*/
	//Do math to format our scoreboard value
	unsigned int value = (blues * 100) + opp;

	Serial.print("writing ");
	switch (display)
	{
		case Score_Screen:
			Serial.print("score: ");
			Serial.println(value);
			scoreDisplay.print(value, DEC);
      //assume no one is going to score more than 10 goals
      scoreDisplay.writeDigitNum(0,0);
      scoreDisplay.writeDisplay();
			break;
		case Shots_Screen:
			Serial.print("shots: ");
			Serial.println(value);
			spiSendValue(value);
			break;
		default:
			break;
	}

}

//Given a number, spiSendValue chops up an integer into four values and sends them out over spi
void spiSendValue(int tempCycles)
{
	digitalWrite(CS_PIN, LOW); //Drive the CS pin low to select OpenSegment

	SPI.transfer(tempCycles / 1000); //Send the left most digit
	tempCycles %= 1000; //Now remove the left most digit from the number we want to display
	SPI.transfer(tempCycles / 100);
	tempCycles %= 100;
	SPI.transfer(tempCycles / 10);
	tempCycles %= 10;
	SPI.transfer(tempCycles); //Send the right most digit

	digitalWrite(CS_PIN, HIGH); //Release the CS pin to de-select OpenSegment
}

void NewBoxScore(BoxScore boxScore) {
	//Eventually put logic in here to 'smartly' update Displays

	UpdateDisplay(boxScore.bluesGoals, boxScore.oppGoals, Score_Screen);
	UpdateDisplay(boxScore.bluesSog, boxScore.oppSOG, Shots_Screen);
}

BoxScore ParseMessage(char* message) {
	BoxScore ret;
	const size_t bufferSize = JSON_OBJECT_SIZE(7) + 100;
	DynamicJsonBuffer jsonBuffer(bufferSize);

	JsonObject& root = jsonBuffer.parseObject(message);

	bool BluesHome = root["BluesHome"]; 
	short homeScore = root["HomeScore"]; 
	short homeSOG = root["HomeSOG"]; 
	short awayScore = root["AwayScore"]; 
	short awaySOG = root["AwaySOG"]; 
	int timeRemaining = root["TimeRemaining"]; 
	short period = root["Period"]; 

	ret.bluesGoals = (BluesHome)? homeScore: awayScore;
	ret.bluesSog = (BluesHome) ? homeSOG : awaySOG;
	ret.oppGoals = (!BluesHome) ? homeScore : awayScore;
	ret.oppSOG = (!BluesHome) ? homeSOG : awaySOG;
	//ret.timeRemaining = timeRemaining;.

	return ret;
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

