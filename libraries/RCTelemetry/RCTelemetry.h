/**
 * Library......: RC Telemetry
 * Author.......: Michael Rouse (2016)
 * Description..: Contains the data structure, variables, and functions used by both the RC Telemetry transmitter and receiver 
 */
#ifndef RCTELEM_h
#define RCTELEM_h

#include "Arduino.h"

/**
 * Constants
 */
#define BAUD_RATE           9600                    // Baud rate for serial communication ( back to computer, for debugging )
#define RADIO_CHANNEL       108                     // Channel for radio communication
#define RADIO_ADDRESS       0xF0F0F0F0E1LL          // Address for Transmitter to TX on, Receiver to RX on
#define TELEM_PACKET_SIZE   sizeof(TelemetryData)   // Size of a telemetry data packet ( does not get evaluated until compile, so you can use TelemetryData
                                                    // even though it hasn't been defined yet )
#define TX_BEAT             200                     // Milliseconds between each transmission

#define GPS_RX_PIN          4                       // Pin to receive GPS data (goes to TX pin on GPS)
#define GPS_TX_PIN          3                       // Pin to transmit data to GPS (goes to RX pin on GPS) 

#define MIN_SATS            4                       // Minimum number of satellites required to run

#define NO_GPS_ERROR        "ERR1"                  // No gps found error message
#define NO_SATS_ERROR       "ERR2"                  // No satellites found error message

/**
 * Data Type....: Telemetry Data 
 * Author.......: Michael Rouse
 * Description..: All Telemetry Data  
 */
struct TelemetryData
{
  double topSpeed;       // Maximum speed achieved
  double speed;          // Current speed at any given time 
  
  double startAltitude;  // Starting altitude
  double maxAltitude;    // Maximum altitude acheived
  double altitude;       // Current altitude at any given time
  
  double latitude;       // Latitude location 
  double longitude;      // Longitude location 
};




#endif