/**
 * Program......: Arduino Telemetry Reciever
 * Author.......: Michael Rouse
 * Description..: Arduino Code for the telemetry reciever
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <EEPROM.h>
#include "MEMORY.h"
#include "printf.h"
#include <RCTelemetry.h>

/**
 * Global Variables
 */
RF24 radio(9, 10); // Setup the Radio on SPI Bus, as well as pins 9 and 10

TelemetryData data; // Holds incoming telemetry packet data



/**
 * Function.....: Setup
 * Description..: Runs once when Arduino is powered on
 */
void setup() 
{
  Serial.begin(BAUD_RATE);

  // Radio Setup
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(RADIO_CHANNEL);
  radio.openReadingPipe(1, RADIO_ADDRESS);  // For receiving telemetry
  radio.startListening();
  
  Serial.println("Telemetry Receiver Ready\n");
}

/**
 * Function.....: Loop
 * Description..: Main program, runs as long as the Arduino is powerd on
 */
void loop() 
{
  if (radio.available()) // Check for incoming data from transmitter
  {
    while (!radio.read(&data, TELEM_PACKET_SIZE));  // Receive the data
    
    // Print the data
    Serial.print(data.topSpeed); Serial.print(",");
    Serial.print(data.speed); Serial.print(",");
    Serial.print(data.maxAltitude); Serial.print(",");
    Serial.print(data.altitude); Serial.print(",");
    Serial.print(data.latitude, 6); Serial.print(",");
    Serial.print(data.longitude, 6); Serial.print("\n");
  }
}

