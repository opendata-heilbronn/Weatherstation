#include <Arduino.h>i
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
#include "DHT.h"

#include "config.h"

#define maxBuffer 80
WiFiClient client;
#define SERIALOUT 1


DHT dht(D2, DHT11);

SoftwareSerial mySerial(D8, D7); // RX, TX

int incomingByte = 0;


char inputBuffer[maxBuffer];   // For incoming serial data
int state1 = 0;


void setup() {
  pinMode(D4, OUTPUT);
  digitalWrite(D4, LOW);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  delay(1000);

  if (SERIALOUT) {
    Serial.begin(38400);
    Serial.println("Start Platform IO");
  }
  mySerial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    toggleLed();
    if (SERIALOUT) {
      Serial.print(".");
    }
  }

  if (SERIALOUT) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  dht.begin();

  delay(5000);
  digitalWrite(D4, HIGH);
  turnOffLed();
}

void toggleLed() {
  if (state1 == 0) {
    state1 = 1;
    digitalWrite(D0, HIGH);
  } else {
    state1 = 0;
    digitalWrite(D0, LOW);
  }
}

void turnOnLed() {
    state1 = 1;
    digitalWrite(D0, HIGH);  
}

void turnOffLed() {
    state1 = 0;
    digitalWrite(D0, LOW);
}

void sendData(String data, int numData) {
  if (client.connect(server, 80)) { 
    //Serial.println(F("connected to server"));
    // Make a HTTP request:
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray& array = jsonBuffer.createArray();

    for(int i = 0; i < numData; i++){
      JsonObject& sensor = array.createNestedObject();
        
      sensor["type"] = "elv";
      sensor["value"] = data.substring(i*16, i*16+15);
    }

    String sensorJson = String("POST /rawSensor HTTP/1.0\r\nHost: "+hostName+"\r\nContent-Type: application/json\r\nConnection: close\r\n");

    int len = array.measureLength();
    sensorJson += "Content-Length: ";
    sensorJson += len;
    sensorJson += "\r\n\r\n";
    array.printTo(sensorJson);

    client.print(sensorJson);
    client.stop();
    if (SERIALOUT) {
      Serial.println(sensorJson);
      Serial.println("finished");
    }
  }
}

void sendTempSensor(float temp, float humidity) {
  if (client.connect(server, 80)) { 
    //Serial.println(F("connected to server"));
    // Make a HTTP request:
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray& array = jsonBuffer.createArray();

    JsonObject& sensor1 = array.createNestedObject();
    sensor1["sensorName"] = "cowo.inside3.temperature";
    sensor1["value"] = temp;

    JsonObject& sensor2 = array.createNestedObject();
    sensor2["sensorName"] = "cowo.inside3.humidity";
    sensor2["value"] = humidity;

    String sensorJson = String("POST /sensor HTTP/1.0\r\nHost: "+hostName+"\r\nContent-Type: application/json\r\nConnection: close\r\n");

    int len = array.measureLength();
    sensorJson += "Content-Length: ";
    sensorJson += len;
    sensorJson += "\r\n\r\n";
    array.printTo(sensorJson);

    client.print(sensorJson);
    client.stop();
    if (SERIALOUT) {
      Serial.println(sensorJson);
      Serial.println("finished");
    }
  }
}

unsigned long lastLocalSensorTime = 0;
unsigned long lastSerial = 0;

void readAndSendLocalSensor() {
    turnOnLed();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    Serial.println("Temp: "+String(temperature)+ " Humidity: "+String(humidity));

    if (humidity > 0) {
      sendTempSensor(temperature, humidity);
    }
    lastLocalSensorTime = millis();
    turnOffLed();
}

// the loop function runs over and over again forever
void loop() {
  //toggleLed();

  long diff = lastLocalSensorTime - millis();
  if (abs(diff) > 150*1000) {
    long timeToReadSensor = millis();
    Serial.println("Sending local sensor values... ");
    readAndSendLocalSensor();
    long diffReadTime = millis() - timeToReadSensor;
    Serial.println("Time to read sensor: "+String(diffReadTime)+" ms");
  }  
/*  if (SERIALOUT) {
    Serial.println(String(millis()) + "  -  "+String(abs(diff)));
  }*/
  if((millis() - lastSerial) > 300000){
    mySerial.write("S");

    delay(1);

    int count = 0;
    String sensorData = "";
    
    while (mySerial.available() > 8) {

      byte TEMP[8];
      for(int i = 0; i < 8; i++){
        TEMP[i] = mySerial.read();
      }

      if(TEMP[0] != 0x02 && TEMP[7] != 0x03)
        continue;
      
      for (int x = 0; x < 8; x++) {
        if (inputBuffer[x] <= 0xf) {
          sensorData += "0";
        }
        sensorData += String(TEMP[x], HEX);
      }
      count++;
    }

    
    turnOnLed();
    sendData(sensorData, count);
    turnOffLed();
    if (SERIALOUT) {
      Serial.println("Time: "+String(millis()));
      Serial.println(sensorData);
      Serial.println("----");
    }

    lastSerial = millis();
  }
  delay(1);
}
