/**
 * Program......: Arduino Telemetry
 * Author.......: Michael Rouse
 * Description..: Arduino Code for the telemetry unit that goes on the
 *                RC car, boat, or plane
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "MEMORY.h"
#include "printf.h"
#include <RCTelemetry.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>



/**
 * Constants
 */
#define DISPLAY_ADDRESS   0x70

#define ALL_TIME_SPEED_LOCATION   190     // Location in EEPROM for the all-time top speed
#define SPEED_SAVE_LOCATION       210     // Location in EEPROM to save the top speed

#define ALL_TIME_ALT_LOCATION     150     // Location in EEPROM to save the all-time max altitude
#define ALT_SAVE_LOCATION         170     // Location in EEPROM to save the max altitude


// States that this program can be in 
#define TOP_SPEED_STATE            1       // State that will show the top speed on the screen (default)
#define MAX_ALT_STATE              2       // State that will show the max altitude reached 

// Button for resetting and controlling the state
#define BUTTON_RESET_TIME         2000    // Hold the button for 2,000 ms to reset the value of whatever state the program is in
#define STATE_BUTTON              6       // Digital Pin that the state button is on


/**
 *  Global Variables
 */
TinyGPSPlus gps;                                    // GPS object
SoftwareSerial gps_ss(GPS_TX_PIN, GPS_RX_PIN);      // Setup software serial for the GPS

Adafruit_7segment display   = Adafruit_7segment();  // 7-segment display

RF24 radio(9, 10);                                  // Setup the Radio on SPI Bus, as well as pins 9 and 10

TelemetryData data;                                 // Holds outgoing telemetry packet data

unsigned long tx_timer;                            // Keeps track of when to transmit

//float maximumSpeed  = memoryRead(ALL_TIME_SPEED_LOCATION);
//float maximumAlt    = memoryRead(ALL_TIME_ALT_LOCATION);

char label[] = "0SAT";

// State controls
unsigned long button_press_ms   = 0;                // Used for determining if the button has been longpressed
int last_button_state           = 0;                // Last known state of the button
int button_state                = 0;                // Current state of the button  
int state                       = TOP_SPEED_STATE;  // Current state



/**
 * Function.....: Setup
 * Description..: Runs once when Arduino is powered on
 */
void setup()
{

    Serial.begin(BAUD_RATE);
    Serial.println("DEBUG MODE ENABLED");

  
  gps_ss.begin(BAUD_RATE); // Start SoftwareSerial for GPS

  display.begin(DISPLAY_ADDRESS);  // Initialize 7-segment display
  
  // Output statistics
  //Serial.print("All-time top speed: "); Serial.println(maximumSpeed);
  //Serial.print("All-time max altitude: "); Serial.println(maximumAlt);
  Serial.print("\n");
  //Serial.print("Previous max speed: "); Serial.println(memoryRead(SPEED_SAVE_LOCATION));
  //Serial.print("Previous max altitude: "); Serial.println(memoryRead(ALT_SAVE_LOCATION));
  Serial.print("\n");
  
  // Radio Setup
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(RADIO_CHANNEL);
  radio.openWritingPipe(RADIO_ADDRESS);    // For Transmitting data
  radio.startListening();
  
  // Display "hold" on the output 
  display.displayString("HOLD");
  
  // Set the state/reset button as input 
  pinMode(STATE_BUTTON, INPUT);

  // Default values for telemetry data
  data.speed          = 0.00;
  data.topSpeed       = 0.00;
  data.startAltitude  = 0.00;
  data.altitude       = 0.00;
  data.maxAltitude    = 0.00;
  data.latitude       = 0.00;
  data.longitude      = 0.00;

  
  // Do not start until at least MIN_SATS GPS satellites are locked
  unsigned long last_gps_beat = millis();
  
  do 
  { 
    Serial.println(gps_ss.available());
    // Get GPS data if it exists
    while (gps_ss.available() > 0)
    {
      if(gps.encode(gps_ss.read()))
      {
        // Display number of sats on the display output
        label[0] = (char)(gps.satellites.value() + 48); display.displayString(label);

      
          Serial.println(gps.satellites.value());
    
      }

      last_gps_beat = millis(); // Record the last read of GPS characters
    } // End while
   
    // Show no GPS error if 6 seconds have passed since the last processing of characters from the GPS
    if ((millis() - last_gps_beat) > 60000) 
    {
      showError(NO_GPS_ERROR);
    }

    // Show no sats error after 5 minutes if still in the loop
    if (millis() > 300000000 && gps.satellites.value() < MIN_SATS)
    {
      showError(NO_SATS_ERROR);
    }
  } while (gps.satellites.value() < MIN_SATS); // End requirement for number of satellites
  
  Serial.println((millis() / 1000) / 60); // Show the amount of time, in minutes, it took to get the minimum number of satellites required
  delay(1000);
  
  // Display "Redy" on the ouput
  display.displayString("REDY");
  delay(1000);

  updateDisplay(); 
  
  tx_timer = millis();  // Start Transmit timer
}


/**
 * Function.....: Loop
 * Description..: Main program, runs as long as the Arduino is powerd on
 */
void loop()
{
  // Transmit data every TX_BEAT milliseconds
  if (millis() > tx_timer)
  {
    tx_timer = millis() + TX_BEAT;  // Increase the timer for the next tx beat
    transmit();                     // Transmit telemetry
  }

  // Gather data from GPS if it is available
  while (gps_ss.available() > 0)
  {
    if(gps.encode(gps_ss.read()))
    {
      gatherData(); // Handle new GPS data
    }
  }
  
  // Check for a press in the state button
  checkButtonPress();
}


/**
 *  Function.....: Gather Data
 *  Description..: Updates Telemetry Data
 */
void gatherData()
{
  // Update location if it's available
  if (gps.location.isUpdated())
  {
    if (gps.location.isValid())
    {
      data.latitude = gps.location.lat();
      data.longitude = gps.location.lng();
    }
  }
  
  // Update altitude if it's available
  if (gps.altitude.isUpdated())
  {
    data.altitude = gps.altitude.feet();

    // Set base altitude 
    if (data.startAltitude == 0.000000) { data.startAltitude = data.altitude; }

    // Save altitude
    if ((data.altitude - data.startAltitude) > data.maxAltitude)
    {
      data.maxAltitude = (data.altitude - data.startAltitude);
      
      // Save all-time highest altitude
      //f (data.maxAltitude > maximumAlt)
      //{
        //maximumAlt = data.maxAltitude;
        //memoryWrite(ALL_TIME_ALT_LOCATION, maximumAlt); // Save highest altitude ever to EEPROM
      //}

      // Update stuff on the display
      updateDisplay();
    }
  }
  
  // Update speed if available
  if (gps.speed.isUpdated())
  {
    data.speed = gps.speed.mph();

    // Save top speed
    if (data.speed > data.topSpeed)
    {
      data.topSpeed = data.speed;

      // Save all-time top speed
      //if (data.topSpeed > maximumSpeed)
      //{
        //maximumSpeed = data.topSpeed;
        //memoryWrite(ALL_TIME_SPEED_LOCATION, maximumSpeed); // Save highest speed ever to EEPROM
     // }

      // Update stuff on the display
      updateDisplay();
    }
  }
  
  smartDelay(0);
  
  return;
}


/**
 *  Function.....: Transmit
 *  Description..: Transmits data
 */
void transmit()
{
  radio.stopListening();
  radio.write(&data, TELEM_PACKET_SIZE);  // Send the data
  radio.startListening();

  return;
}


/**
 * Function.....: Check Button Press
 * Description..: Checks for a change in the button
 */
void checkButtonPress()
{
  // Get the current button state 
  button_state = digitalRead(STATE_BUTTON);
  bool long_pressed = false;
  
  // Check for a change 
  if (button_state != last_button_state)
  {
    last_button_state = button_state;
    
    button_press_ms = millis(); // Record the time that the button was pressed at 
    
    // Loop until the button is released or the long press button time is reached
    do 
    {
      button_state = digitalRead(STATE_BUTTON);
      long_pressed = (millis() - button_press_ms) > BUTTON_RESET_TIME;  // Will be true when the button has been pressed for BUTTON_RESET_TIME milliseconds
      delay(50);
    } while ((button_state == last_button_state) && !long_pressed);
    
    // Check how long it was until the button was released
    if (long_pressed)
    {
      // Button was long pressed, reset the current max value
      resetStateValue();
    }
    else 
    {
      // Button was not long pressed, change the state 
      changeState();
    }

    last_button_state = digitalRead(STATE_BUTTON);
  }
  
  return;
}


/**
 * Function.....: Change State
 * Description..: Changes the state of the program 
 */
void changeState()
{
  state++; // Move to the next state

  // Show the label for the new state
  if (state == TOP_SPEED_STATE)
  {
    display.displayString("TOP");
    delay(1000);
    display.displayString("SPD");
  }
  else if (state == MAX_ALT_STATE)
  {
    display.displayString("MAX");
    delay(1000);
    display.displayString("ALT");
  }
  else 
  {
    // Not a valid state, reset to TOP_SPEED_STATE state 
    state = TOP_SPEED_STATE - 1;
    changeState();
  }
 
  delay(1000);

  updateDisplay();
  
  return;
}


/**
 * Function.....: Reset State Value
 * Description..: Resets the current value of whatever state the program is in 
 */
void resetStateValue()
{
  display.displayString("RST");

  // Wait for the button to be released 
  do 
  {
    button_state = digitalRead(STATE_BUTTON);
    delay(50);
  } while (button_state == last_button_state);

  // Reset the value according to the current state
  if (state == TOP_SPEED_STATE)
  {
    data.topSpeed = 0;
  }
  else if (state == MAX_ALT_STATE)
  {
    data.maxAltitude = 0;
  }
  
  delay(1000);

  updateDisplay();
  
  return;
}


/** 
 *  Function.....: Update Display
 *  Description..: Updates the contents of the display 
 */
void updateDisplay()
{
  if (state == TOP_SPEED_STATE)
  {
    display.printFloat(data.topSpeed, 1);
  }
  else if (state == MAX_ALT_STATE)
  {
    display.printFloat(data.maxAltitude, 1);
  }
 
  display.writeDisplay();
  
  return;
}


/**
 * Function.....: Show error
 * Description..: Shows an error message and loops forever
 */
void showError(char error[5])
{
  Serial.println(error);
  display.displayString(error); // Show the error
  while(1){delay(50);} // Loop forever
}


/**
 * Function.....: Read
 * Description..: Reads from EEPROM
 */
double memoryRead(unsigned int location)
{
  double val = 0;
  EEPROM_read(location, val);
  return val;
}


/**
 * Function.....: Memory Write
 * Description..: Writes to EEPROM
 */
void memoryWrite(unsigned int location, double val)
{
  EEPROM_write(location, val);
 
  return;
}


/**
 * Function.....: GPS Smart Delay
 * Description..: Some stuff for "feeding" the GPS
 */
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (gps_ss.available())
      gps.encode(gps_ss.read());
  } while (millis() - start < ms);

  return;
}

