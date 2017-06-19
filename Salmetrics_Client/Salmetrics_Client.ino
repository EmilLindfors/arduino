/*
  LoRa Simple Client for Arduino :
  Support Devices: LoRa Shield + Arduino 
  
  Example sketch showing how to create a simple messageing client, 
  with the RH_RF95 class. RH_RF95 class does not provide for addressing or
  reliability, so you should only use RH_RF95 if you do not need the higher
  level messaging abilities.

  It is designed to work with the other example LoRa Simple Server

  modified 16 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>  


#define TOTAL_CIRCUITS 2                       // <-- CHANGE THIS | set how many I2C circuits are attached to the Tentacle

const unsigned int baud_host  = 9600;        // set baud rate for host serial monitor(pc/mac/other)
const unsigned int send_readings_every = 10000; // set at what intervals the readings are sent to the computer (NOTE: this is not the frequency of taking the readings!)
unsigned long next_serial_time;

char sensordata[30];                          // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;               // We need to know how many characters bytes have been received
byte code = 0;                                // used to hold the I2C response code.
byte in_char = 0;                             // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.

int channel_ids[] = {97,100};        // <-- CHANGE THIS. A list of I2C ids that you set your circuits to.
char *channel_names[] = {"DO","EC"};   // <-- CHANGE THIS. A list of channel names (must be the same order as in channel_ids[]) - only used to designate the readings in serial communications
String readings[TOTAL_CIRCUITS];               // an array of strings to hold the readings of each channel
int channel = 0;                              // INT pointer to hold the current position in the channel_ids/channel_names array

const unsigned int reading_delay = 1000;      // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_reading_time;              // holds the time when the next reading should be ready from the circuit
boolean request_pending = false;              // wether or not we're waiting for a reading

const unsigned int blink_frequency = 250;     // the frequency of the led blinking, in milliseconds
unsigned long next_blink_time;                // holds the next time the led should change state
boolean led_state = LOW;                      // keeps track of the current led state





// Singleton instance of the radio driver
RH_RF95 rf95;
float frequency = 868.0;

void setup() 
{

 pinMode(13, OUTPUT);                        // set the led output pin
  Wire.begin();                   // enable I2C port.
  next_serial_time = millis() + send_readings_every;  // calculate the next point in time we should do serial communications


  
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  Serial.println("Start LoRa Client");
  if (!rf95.init())
    Serial.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
}

void loop()
{
  do_sensor_readings();
  do_serial();


  
  Serial.println("Sending to LoRa Server");
  // Send a message to LoRa Server
  /*
  uint8_t data[] = "Hello World!";
  rf95.send(data, sizeof(data));
  
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is LoRa server running?");
  }
  */
  delay(10000);
}




// do serial communication in a "asynchronous" way
void do_serial() {
  if (millis() >= next_serial_time) {                // is it time for the next serial communication?
    for (int i = 0; i < TOTAL_CIRCUITS; i++) {       // loop through all the sensors
      
  uint8_t data[30];
  //Serial.println(channel_names[i]);  
  //Serial.println(readings[i]);
  
  //String data2 = i+","+readings[i];
  
  Serial.println(data2);
  data2.toCharArray(data,30);
  rf95.send(data, sizeof(data));
  
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

    }
    next_serial_time = millis() + send_readings_every;
  }
}



// take sensor readings in a "asynchronous" way
void do_sensor_readings() {
  if (request_pending) {                          // is a request pending?
    if (millis() >= next_reading_time) {          // is it time for the reading to be taken?
      receive_reading();                          // do the actual I2C communication
    }
  } else {                                        // no request is pending,
    channel = (channel + 1) % TOTAL_CIRCUITS;     // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
    request_reading();                            // do the actual I2C communication
  }
}



// Request a reading from the current channel
void request_reading() {
  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]); // call the circuit by its ID number.
  Wire.write('r');                    // request a reading by sending 'r'
  Wire.endTransmission();                   // end the I2C data transmission.
  next_reading_time = millis() + reading_delay; // calculate the next time to request a reading
}



// Receive data from the I2C bus
void receive_reading() {
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();

  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.

    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }

  switch (code) {                       // switch case based on what the response code is.
    case 1:                             // decimal 1  means the command was successful.
      readings[channel] = sensordata;
      break;                              // exits the switch case.

    case 2:                             // decimal 2 means the command has failed.
      readings[channel] = "error: command failed";
      break;                              // exits the switch case.

    case 254:                           // decimal 254  means the command has not yet been finished calculating.
      readings[channel] = "reading not ready";
      break;                              // exits the switch case.

    case 255:                           // decimal 255 means there is no further data to send.
      readings[channel] = "error: no data";
      break;                              // exits the switch case.
  }
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}                       // is a request pending?

