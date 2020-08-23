//******************************************************************************************
//  File: ST_Anything_Sleepy_ESP8266WiFi.ino
//  Authors: Dan G Ogorchock & Daniel J Ogorchock (Father and Son)
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of a ESP8266 to implement Deep Sleep so it
//            can be used with battery power like a standard wireless sensor.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the ESP8266's WiFi.
//
//            ST_Anything_Sleepy_ESP8266WiFi implements the following ST Capabilities as a demo of what is possible with a single ESP8266
//              - 1 x Temperature Measurement device (Temperature from DHT22 device)
//              - 1 x Humidity Measurement device (Humidity from DHT22 device)
//    
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2017-08-14  Dan Ogorchock  Original Creation - Adapted from ESP8266 to work with ESP32 board
//    2018-02-09  Dan Ogorchock  Added support for Hubitat Elevation Hub
//    2020-08-06  Allan (vseven) Added support for BlueTooth logging
//    2020-08-10  Allan (vseven) Added support for deep sleep on the ESP32
//    2020-08-22  a00889920      Added power savings to ESP8266 to be more efficient when running on batteries
//
//   Special thanks to Joshua Spain for his contributions in porting ST_Anything to the ESP32!
//
//******************************************************************************************
//******************************************************************************************
// SmartThings Library for ESP8266WiFi
//******************************************************************************************
#include <SmartThingsESP8266WiFi.h>


//******************************************************************************************
// ST_Anything Library 
//******************************************************************************************
#include <Constants.h>       //Constants.h is designed to be modified by the end user to adjust behavior of the ST_Anything library
#include <Device.h>          //Generic Device Class, inherited by Sensor and Executor classes
#include <Sensor.h>          //Generic Sensor Class, typically provides data to ST Cloud (e.g. Temperature, Motion, etc...)
#include <Executor.h>        //Generic Executor Class, typically receives data from ST Cloud (e.g. Switch)
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications

#include <PS_TemperatureHumidity.h>  //Implements a Polling Sensor (PS) to measure Temperature and Humidity via DHT library

//*************************************************************************************************
//NodeMCU v1.0 ESP8266-12e Pin Definitions (makes it much easier as these match the board markings)
//*************************************************************************************************
//#define LED_BUILTIN 16
//#define BUILTIN_LED 16
//
//#define D0 16  //no internal pullup resistor
//#define D1  5
//#define D2  4
//#define D3  0  //must not be pulled low during power on/reset, toggles value during boot
//#define D4  2  //must not be pulled low during power on/reset, toggles value during boot
//#define D5 14
//#define D6 12
//#define D7 13
//#define D8 15  //must not be pulled high during power on/reset

//******************************************************************************************
//Define which Arduino Pins will be used for each device
//******************************************************************************************
#define PIN_TEMPERATUREHUMIDITY_1         D7  //SmartThings Capabilities "Temperature Measurement" and "Relative Humidity Measurement"

//******************************************************************************************
//Smarthings ESP8266 will perform extra steps to reduce the power consumed as much as possible:
// Disabling WiFi when waking up
// Disabling network persistence
// Avoiding network scan
// Using WAKE_RF_DISABLED
//******************************************************************************************
#define RUNNING_ON_BATTERY 

//******************************************************************************************
//Define some sleep settings.  
//******************************************************************************************
#define uS_TO_S_FACTOR 1000000    // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  3600       // Time ESP32 will go to sleep (in seconds).  3600 = 1 hour.  Set higher for better 
                                  // battery life, lower for more frequent updates.
RTC_DATA_ATTR int bootCount = 0;  // Save the boot count to RC memory which will survive sleep.

//******************************************************************************************
//ESP8266 WiFi Information
//******************************************************************************************
String str_ssid     = "YourSSIDHere";                           //  <---You must edit this line!
String str_password = "YourPasswordHere";                   //  <---You must edit this line!
IPAddress ip(192, 169, 1, 129);       //Device IP Address       //  <---You must edit this line!
IPAddress gateway(192, 168, 1, 1);    //Router gateway          //  <---You must edit this line!
IPAddress subnet(255, 255, 255, 0);   //LAN subnet mask         //  <---You must edit this line!
IPAddress dnsserver(192, 168, 1, 1);  //DNS server              //  <---You must edit this line!
const unsigned int serverPort = 8090; // port to run the http server on

// Smarthings Hub Information
IPAddress hubIp(192, 168, 1, 149);  // smartthings hub ip       //  <---You must edit this line!
const unsigned int hubPort = 39500; // smartthings hub port

// Hubitat Hub Information
//IPAddress hubIp(192, 168, 1, 143);    // hubitat hub ip         //  <---You must edit this line!
//const unsigned int hubPort = 39501;   // hubitat hub port


//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
//  String strTemp = msg;
//  Serial.print(F("ST_Anything Callback: Sniffed data = "));
//  Serial.println(msg);
//  SerialBT.print(F("ST_Anything Callback: Sniffed data = "));  // BlueTooth monitoring
//  SerialBT.println(msg);

  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
//  if (strTemp.startsWith("temperature1"))
//  {
//    strTemp.remove(0,13);
//    Serial.println(strTemp);
//  }
//  if (strTemp.startsWith("humidity1"))
//  {
//    strTemp.remove(0,10);
//    Serial.println(strTemp);
//  }
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
  st::Everything::preInit();  // This should be the first thing called in setup
  Serial.println("I'm awake.");

  //******************************************************************************************
  //Declare each Device that is attached to the Arduino
  //  Notes: - For each device, there is typically a corresponding "tile" defined in your 
  //           SmartThings Device Hanlder Groovy code, except when using new COMPOSITE Device Handler
  //         - For details on each device's constructor arguments below, please refer to the 
  //           corresponding header (.h) and program (.cpp) files.
  //         - The name assigned to each device (1st argument below) must match the Groovy
  //           Device Handler names.  (Note: "temphumid" below is the exception to this rule
  //           as the DHT sensors produce both "temperature" and "humidity".  Data from that
  //           particular sensor is sent to the ST Hub in two separate updates, one for 
  //           "temperature" and one for "humidity")
  //         - The new Composite Device Handler is comprised of a Parent DH and various Child
  //           DH's.  The names used below MUST not be changed for the Automatic Creation of
  //           child devices to work properly.  Simply increment the number by +1 for each duplicate
  //           device (e.g. contact1, contact2, contact3, etc...)  You can rename the Child Devices
  //           to match your specific use case in the ST Phone Application.
  //******************************************************************************************
  //Polling Sensors
  static st::PS_TemperatureHumidity sensor2(F("temphumid1"), 15, 5, PIN_TEMPERATUREHUMIDITY_1, st::PS_TemperatureHumidity::DHT22,"temperature1","humidity1");

  //Interrupt Sensors 
  
  //Special sensors/executors (uses portions of both polling and executor classes)
  
  //Executors
  
  //*****************************************************************************
  //  Configure debug print output from each main class 
  //  -Note: Set these to "false" if using Hardware Serial on pins 0 & 1
  //         to prevent communication conflicts with the ST Shield communications
  //*****************************************************************************
  st::Everything::debug=true;
  st::Executor::debug=true;
  st::Device::debug=true;
  st::PollingSensor::debug=true;
  st::InterruptSensor::debug=true;

  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************

  //Initialize the optional local callback routine (safe to comment out if not desired)
  st::Everything::callOnMsgSend = callback;
  
  //Create the SmartThings ESP8266WiFi Communications Object
    //STATIC IP Assignment - Recommended
    st::Everything::SmartThing = new st::SmartThingsESP8266WiFi(str_ssid, str_password, ip, gateway, subnet, dnsserver, serverPort, hubIp, hubPort, st::receiveSmartString);
 
    //DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
    //st::Everything::SmartThing = new st::SmartThingsESP8266WiFi(str_ssid, str_password, serverPort, hubIp, hubPort, st::receiveSmartString);

  //Run the Everything class' init() routine which establishes WiFi communications with SmartThings Hub
  st::Everything::init();
  
  //*****************************************************************************
  //Add each sensor to the "Everything" Class
  //*****************************************************************************
  st::Everything::addSensor(&sensor1);
        
  //*****************************************************************************
  //Add each executor to the "Everything" Class
  //*****************************************************************************

  //*****************************************************************************
  //Increment our boot count, find out why we booted up, then setup sleep
  //*****************************************************************************
  ++bootCount;
  delay(500);
  Serial.println("**************************************");
  Serial.println("Boot number: " + String(bootCount));
  Serial.println("Setup ESP8266 to sleep every " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.println("**************************************");
  
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();

  //*****************************************************************************
  //Go to sleep
  //*****************************************************************************
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  st::Everything::deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  // st::Everything::run();
}
