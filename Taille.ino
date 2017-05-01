
/* Compute Height using a VL53L0X LIDAR
 *  Display the results on a 128x64 screen
 *  Makes the result available on a web page
 */

// Includes various required libraries.

#include <SPI.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>

// Contant definitions
// WiFi SSID and Password.
const char* ssid = "Fukushima";
const char* password = "soja71760406";

// How long to stay ON before turning off.
const int gotosleep = 60;

// A few defines for the OLED and VL53L0X

#define OLED_RESET 4
#define LONG_RANGE
#define HIGH_ACCURACY



// Global Variables and Objects declaration

SSD1306 display(0x3c, 4, 5);      // Display object is at address 0x3c and requires Pins 4 and 5.
ESP8266WebServer server(80);      // Web Server used for displaying the results.
VL53L0X sensor;                   // LIDAR Sensor.
Ticker switchoff;                 // Timer used to measure time before Deep SLeep.


float height = 0;                 // Measurement of the height of the sensor.
int seconds = gotosleep;          // Initial settings in seconds before we turn off.
bool Tired= false;                // Flag to know if it's time to turn off.


// Initialize the OLED screen and display startup message.

void init_oled_128x64() {
  Serial.println("Initializing Display...");
  display.init();                               // Initialising the UI will init the display too.
  display.flipScreenVertically();               // Flip the screen. 
  display_splash("Welcome !!!");
  set_status(20,"Display initialized...");
}

// Initialize the VL53L0X 
// Perform initial settings and start the measurement.

void init_vl53l0x() {
  set_status(30,"Initializing sensor");
  sensor.init();                        //Actualy initialize the sensor.
  sensor.setTimeout(500);
  set_status(50,"Sensor initialized");

#if defined LONG_RANGE
  set_status(60,"Long Range"); 
  sensor.setSignalRateLimit(0.2);                                 // lower the return signal rate limit (default is 0.25 MCPS)
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);   // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14); // increase laser pulse periods (defaults are 14 and 10 PCLKs)
#endif

#if defined HIGH_SPEED
  sensor.setMeasurementTimingBudget(20000);　 // reduce timing budget to 20 ms (default is about 33 ms)
  set_status(70,"High speed");

#elif defined HIGH_ACCURACY
  set_status(70,"High Accuracy");              // increase timing budget to 200 ms
  sensor.setMeasurementTimingBudget(200000);
#endif

  sensor.startContinuous();                   // Start the measurement.
}

// Start the WiFi 
// Initialize hardware and connect to AP.
// Prepare a WebServer on port 80 and link it to a callback.

void init_wifi() {
  set_status(75,"Connecting Wifi...");
  
  WiFi.persistent(false);                                                                 // Avoid settings being saved to flash if not changed. 
                                                                                          // This could wear out flash quickly.
  WiFi.disconnect();                                                                      // Disconnect any remaining session.
  Serial.printf("Wi-Fi mode set to WIFI_STA %s\n", WiFi.mode(WIFI_STA) ? "" : "Failed!"); // Switch to STATION mode.
  
  WiFi.begin(ssid,password);                                                              // Launch the Wifi connection.
  while (WiFi.status() != WL_CONNECTED) {                                                 // Wait for connection to happen.
    delay(500);
    Serial.print(".");
  }
  set_status(80,"WiFi Connected");
  set_status(85,WiFi.localIP().toString());                                                // Display our IP.
  server.on("/", display_html);                                                            // Attach callback function to ServerRoot.
  server.begin();                                                                         // Start the server
  set_status(90,"Web Server ready !!!");
}

// Function to perform cleanup before shuting-down.

void shutdown_esp() {
  display_splash("Goodbye...");
  display.clear();                                // Clear the screen in case some power remains in Deep Sleep.
  display.display();                              //
  ESP.deepSleep(0);                               // Actually go into Deep Sleep mode... It's over and only RST will restart.
 }

// Display a Progress bar during system init.
// Logs the progress and messages to Serial too.

void set_status(int percentage, String statusmessage) {
  display.setFont(ArialMT_Plain_10);  //default font. Create more fonts at http://oleddisplay.squix.ch/
  display.clear();                    // clear the display
  display.drawProgressBar(0,53,127,10,percentage);
  display.drawString(0, 26, statusmessage);   // draw string (column(0-127), row(0-63, "string")
  display.display();                          //display all you have in display memory
  Serial.println(String (percentage) + "% "+ statusmessage); // Log to Serial also.
  delay(500);
}

// Display a splash screen

void display_splash(String splashmessage) {
  display.clear();                    
  display.setFont(ArialMT_Plain_24);            // Use a big font for startup message.
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64,10,splashmessage);      // Splash message.
  display.display();                            // Actually display the message.
  display.setTextAlignment(TEXT_ALIGN_LEFT);    // Make sure next messages will not be centered.
  delay(2000);
}

// Web Server callback function.
// Display the measured height to the client.

void display_html () {
  String Message;

  if (height == 0) {
    Message = "Server Ready !"; // If we haven't measured anything...
  } else {
    Message = "<h1>Size</h1><P><HR>" + String(height) + " m <P><HR>"; // The message 
  }
  server.send(200,"text/html", Message); // Actually sent response to the client's browser.
}

//Timer callback function.
//It will be called every second and will decrease "seconds".
//When the countdown is over, it sets the "Tired" flag to "true"
//and stop the countdown on further calls.

void timer_clock(){
  if ( seconds > 0) {
    seconds--;
  } else {
    Tired=true;
  }
}

// Display the size given as parameter.
// Also display IP and remaining time before shutdown.

void display_height( float scannedsize) {
  display.clear();
  display.setFont(ArialMT_Plain_10);  //default font. Create more fonts at http://oleddisplay.squix.ch/
  display.drawString(0,0,"Size");
  display.drawHorizontalLine(0,12,127);
  display.setFont(ArialMT_Plain_24);  //default font. Create more fonts at http://oleddisplay.squix.ch/
  display.drawString(30,20,String(scannedsize,2) + "m");
  display.drawHorizontalLine(0,53,127);
  display.setFont(ArialMT_Plain_10);  //default font. Create more fonts at http://oleddisplay.squix.ch/
  display.drawString(0,54,WiFi.localIP().toString());
  display.drawString(100,54,String(seconds) + "s");
  display.display();
}

// Init the ESP and subsystems.

void setup() {
  Wire.begin(4,5);                // Make sure to start I2C on Pins 4 and 5.
  Serial.begin(115200);           // Configure Serial with proper speed.
  delay(10);
  Serial.println("Starting...");

  init_oled_128x64();               // Configure screen.
  init_vl53l0x();                   // Configure sensor.
  init_wifi();                      // Configure network.
  set_status(100,"Ready !!!");
  delay(2000);
  switchoff.attach(1,timer_clock);   // start the timer: execute the callback timer_clock every second.
}

// The main loop:
// 1. Get measurement from sensor.
// 2. Display the refreshed information.
// 3. Answer to HTTP clients.
// 4. If it's time to sleep, get a nap.

void loop() {

  if (sensor.timeoutOccurred() == 0) {
    height = sensor.readRangeContinuousMillimeters();   // Distance measurement
    height = (height/10 + 0.1)/100;                     // Convert from millimeter to meter.
    if (height < 2 ) {                                  // If the measurement is correct
      display_height (height);                          // display it !!!
    }
  }  else {
    display.clear();
    display.drawString(0,0,"Timeout!!");
    display.display();
    delay(1000);
  }
  server.handleClient();                                // Make sure we answer to HTTP clients...
  if (Tired == true) {                                  // If it's time to sleep...
    shutdown_esp();                                     // get a nap...
  }
}




