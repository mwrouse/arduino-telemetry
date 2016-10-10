import processing.core.*; 
import processing.data.*; 
import processing.event.*; 
import processing.opengl.*; 

import processing.serial.*; 

import java.util.HashMap; 
import java.util.ArrayList; 
import java.io.File; 
import java.io.BufferedReader; 
import java.io.PrintWriter; 
import java.io.InputStream; 
import java.io.OutputStream; 
import java.io.IOException; 

public class telemetry_software extends PApplet {




Serial arduino;
PrintWriter output;

float longitude = 0.0f;
float latitude = 0.0f;

float last_longitude = longitude;
float last_latitude = latitude;

float topSpeed = 0.0f;
float speed = 0.0f;

float maxAltitude = 0.0f;
float altitude = 0.0f;


String data = "";


/** 
 *  Function.....: Gather Data 
 *  Description..: Updates Telemetry Data 
 */
public void setup()
{
   
   surface.setTitle("Radio Control Telemetry");

   while (Serial.list().length - 1 < 0)
   {
     println("No COM Devices");
     delay(200);
   }
   
   arduino = new Serial(this, Serial.list()[0], 9600);
   arduino.bufferUntil('\n');
   
   // Create the file with a unique name
   output = createWriter("RCTelemData-" + year() + "-" + month() + "-" + day() + "_" + hour() + "-" + minute() + ".kml");
   
   // Print the KML file starting info
   output.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?><kml xmlns=\"http://www.opengis.net/kml/2.2\"><Document><name>Radio Control Telemetry</name><Style id=\"transPurpleLineGreenPoly\"><LineStyle><color>7fff00ff</color><width>4</width></LineStyle><PolyStyle><color>7f00ff00</color></PolyStyle></Style><Placemark><name>RC Telemetry</name><visibility>1</visibility><description>Radio Control Telemetry</description><styleUrl>#transPurpleLineGreenPoly</styleUrl><LineString><tessellate>1</tessellate><altitudeMode>absolute</altitudeMode><coordinates>");
}

/** 
 *  Function.....: Draw
 *  Description..: Updates the GUI 
 */
public void draw()
{
   background(255);
  
   textSize(26);
   fill(0, 0, 0);
   
   // Label for speed
   text("Speed: ", 78, 60);
   // Speed 
   text(speed, 170, 60);
  
   // label for top speed 
   text("Top Speed:", 20, 100);
   // Top Speed 
   text(topSpeed, 170, 100);
  
   // Label for altitude 
   text("Altitude: ", 54, 160);
   // Altitude 
   text(altitude, 170, 160);
   
   // Label for Max Altitude 
   text("Max Alt: ", 58, 200);
   // Max Altitude 
   text(maxAltitude, 170, 200);
   
   textSize(15);
   text("By: Michael Rouse, 2016", 10, 340);
}


/** 
 *  Function.....: Serial Event
 *  Description..: Handles incoming Serial Data 
 */
public void serialEvent(Serial arduino)
{
 
  data = arduino.readStringUntil(',');
  if (data != null)
  {
    data = data.substring(0, data.length() - 1);
    topSpeed = PApplet.parseFloat(data);
  }
  data = arduino.readStringUntil(',');
  if (data != null)
  {
    data = data.substring(0, data.length() - 1);
    speed = PApplet.parseFloat(data);
  }
  data = arduino.readStringUntil(',');
  if (data != null)
  {
    data = data.substring(0, data.length() - 1);
    maxAltitude = PApplet.parseFloat(data);
  }
  data = arduino.readStringUntil(',');
  if (data != null)
  {
    data = data.substring(0, data.length() - 1);
    altitude = PApplet.parseFloat(data);
  }
  data = arduino.readStringUntil(',');
  if (data != null)
  {
    data = data.substring(0, data.length() - 1);
    latitude = PApplet.parseFloat(data);
  }
  data = arduino.readStringUntil('\n');
  if (data != null)
  {
    longitude = PApplet.parseFloat(data);
  }
  
  // If valid data, output to the file for use in Google Earth
  if (!Float.isNaN(longitude) && !Float.isNaN(latitude) && !Float.isNaN(altitude) && longitude != 0.0f && latitude != 0.0f)
  {
    if (longitude != last_longitude && latitude != last_latitude)
    {
      output.print("\n");
      output.print(longitude);
      output.print(",");
      output.print(latitude);
      output.print(",");
      output.print(toMeters(altitude));
    }
    
    last_latitude = latitude;
    last_longitude = longitude;
  }
  
  
  return;
}


/** 
 *  Function.....: Exit
 *  Description..: Perform this when the app is closed 
 */
public void exit()
{
  output.print("</coordinates></LineString></Placemark></Document></kml>"); // Make the KML file valid
  output.flush();
  output.close(); // Save the output file completely (or something like that)
    
  super.exit();
}


/** 
 *  Function.....: To Meeters
 *  Description..: Converts feet to meters
 */
public float toMeters(float feet)
{
  return feet * 0.3048f;
}

/** 
 *  Function.....: To MPH
 *  Description..: Converts KPH to MPH 
 */
public float toMPH(float kph)
{
  return kph * 0.62137119f;
}
  public void settings() {  size(350, 350); }
  static public void main(String[] passedArgs) {
    String[] appletArgs = new String[] { "telemetry_software" };
    if (passedArgs != null) {
      PApplet.main(concat(appletArgs, passedArgs));
    } else {
      PApplet.main(appletArgs);
    }
  }
}
