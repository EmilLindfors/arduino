/*
  LoRa Simple Yun Server :
  Support Devices: LG01. 
  
  Example sketch showing how to create a simple messageing server, 
  with the RH_RF95 class. RH_RF95 class does not provide for addressing or
  reliability, so you should only use RH_RF95 if you do not need the higher
  level messaging abilities.

  It is designed to work with the other example LoRa Simple Client

  modified 16 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/
//If you use Dragino IoT Mesh Firmware, uncomment below lines.
//For product: LG01. 
#define BAUDRATE 115200

//If you use Dragino Yun Mesh Firmware , uncomment below lines. 
//#define BAUDRATE 250000

#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
//#include "ThingSpeak.h"
#include "YunClient.h"

// Singleton instance of the radio driver
RH_RF95 rf95;

int led = A2;
float frequency = 868.0;


unsigned long myChannelNumber = 283182;
String myWriteAPIKey = "CXVY9M9B8X7KNW7P";
char thingSpeakAddress[] = "api.thingspeak.com";
long lastConnectionTime = 0;
boolean lastConnected = false;

void setup() 
{
  YunClient client;
  pinMode(led, OUTPUT);     
  Bridge.begin(BAUDRATE);
  Console.begin();
  while (!Console) ; // Wait for console port to be available
  Console.println("Start Sketch");
  if (!rf95.init())
    Console.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // Defaults BW Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  Console.print("Listening on frequency: ");
  Console.println(frequency);
  //  ThingSpeak.begin(client);
}

void loop()
{
  if (rf95.available())
  {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      digitalWrite(led, HIGH);
      RH_RF95::printBuffer("request: ", buf, len);

      char* ab = (char)buf;
      String a = String(ab);
      Console.print("Receiving package: ");
      Console.println(a);
      String data = "";

      char* command = strtok(a.c_str(), ";");
      while (command != 0)
      {
        char* separator = strchr(command, ':');
        if (separator != 0)
        {
        *separator = 0;
        //int servoId = atoi(command);
        ++separator;
        //int position = atoi(separator);
        String str1(command);
        str1.replace(";"," ");
        str1.trim();
        String str2(separator);
        str2.replace(";"," ");
        str2.trim();
        Console.print(str1);
        Console.println(str2);
        if (str1=="DO") {
          data.concat("&field1=");
          data.concat(str2);
        }
        if (str1=="EC") {
          data.concat("&field2=");
          data.concat(str2);
        }
        }
        command = strtok(0, "&");
      }
      if (data.length()>0) {
        Console.print("Interpeted as: ");
        Console.println(data);
        updateThingSpeak(data);
      }
      //DO:6.02;EC:0.10
      /*
      int commaIndex = a.indexOf(':');
      String firstValue = a.substring(0, commaIndex);
      String secondValue = a.substring(commaIndex + 1, a.length());
      Console.println(firstValue);
      Console.println(secondValue);
      if (firstValue=="DO") {
        updateThingSpeak("field1=" + secondValue);
        delay(4000);
      } else if (firstValue=="EC") {
        delay(4000);
        updateThingSpeak("field2=" + secondValue);
      } else {
        Console.println("Invalid sensor ID"); 
      }
    */
     
       //updateThingSpeak("field1=" + firstValue + "&field2=" + secondValue);
      digitalWrite(led, LOW);
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}


void updateThingSpeak(String tsData) {
    YunClient client;
  if (client.connect(thingSpeakAddress, 80)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + myWriteAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
    lastConnectionTime = millis();

    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
    }
  }
}


