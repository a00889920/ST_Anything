//******************************************************************************************
//  File: ST_Anything_Multiples_ESP8266WiFi.ino
//  Authors: Dan G Ogorchock & Daniel J Ogorchock (Father and Son)
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings
//            library, demonstrates the ability of one NodeMCU ESP8266 to
//            implement a multi input/output custom device for integration into SmartThings.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the NodeMCU ESP8266's WiFi.
//
//            ST_Anything_Multiples implements the following ST Capabilities as a demo of what is possible with a single NodeMCU ESP8266
//              - 1 x Alarm device (using a simple digital output)
//              - 1 x Contact Sensor devices (used to monitor magnetic door sensors)
//              - 1 x Switch devices (used to turn on a digital output (e.g. LED, relay, etc...)
//              - 1 x Motion devices (used to detect motion)
//              - 1 x Smoke Detector devices (using simple digital input)
//              - 1 x Temperature Measurement devices (Temperature from Dallas Semi 1-Wire DS18B20 device)
//              - 1 x Relay Switch devices (used to turn on a digital output for a set number of cycles And On/Off times (e.g.relay, etc...))
//              - 2 x Button devices (sends "pushed" if held for less than 1 second, else sends "held"
//              - 1 x Water Sensor devices (using the 1 analog input pin to measure voltage from a water detector board)
//
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2015-01-03  Dan & Daniel   Original Creation
//    2017-02-12  Dan Ogorchock  Revised to use the new SMartThings v2.0 library
//    2017-04-17  Dan Ogorchock  New example showing use of Multiple device of same ST Capability
//                               used with new Parent/Child Device Handlers (i.e. Composite DH)
//    2017-05-25  Dan Ogorchock  Revised example sketch, taking into account limitations of NodeMCU GPIO pins
//    2018-02-09  Dan Ogorchock  Added support for Hubitat Elevation Hub
//    2019-11-18  a00889920      Updated code to use new Water_PNP sensor
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
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications

#include <PS_Water_NPN.h>   //Implements a Polling Sensor (PS) to measure presence of water (i.e. leak detector)

//*************************************************************************************************
//NodeMCU v1.0 ESP8266-12e Pin Definitions (makes it much easier as these match the board markings)
//*************************************************************************************************
//#define LED_BUILTIN 16
//#define BUILTIN_LED 16
//
#define D0 16  //no internal pullup resistor
#define D1  5
#define D2  4
#define D3  0  //must not be pulled low during power on/reset, toggles value during boot
#define D4  2  //must not be pulled low during power on/reset, toggles value during boot
#define D5 14
#define D6 12
#define D7 13
#define D8 15  //must not be pulled high during power on/reset

//******************************************************************************************
//Define which Arduino Pins will be used for each device
//******************************************************************************************
#define PIN_WATER_1 A0 //NodeMCU ESP8266 only has one Analog Input Pin 'A0'

#define PIN_WATER_NPN_1 D1 // NPN for wather sensor

//******************************************************************************************
//ESP8266 WiFi Information
//******************************************************************************************
String str_ssid     = "Frontier0624";                               //  <---You must edit this line!
String str_password = "8988317121";                                 //  <---You must edit this line!
IPAddress ip(192, 168, 254, 200);         // Device IP Address      //  <---You must edit this line!
IPAddress gateway(192, 168, 254, 254);    //router gateway          //  <---You must edit this line!
IPAddress subnet(255, 255, 255, 0);       //LAN subnet mask         //  <---You must edit this line!
IPAddress dnsserver(192, 168, 254, 254);  //DNS server              //  <---You must edit this line!
const unsigned int serverPort = 8090;     // port to run the http server on

// Smartthings Hub Information
IPAddress hubIp(192, 168, 254, 123);      // smartthings hub ip     //  <---You must edit this line!
const unsigned int hubPort = 39500;       // smartthings hub port


//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor
//    data being sent to ST.  This allows a user to act on data changes locally within the
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
  //  Serial.print(F("ST_Anything Callback: Sniffed data = "));
  //  Serial.println(msg);

  //TODO:  Add local logic here to take action when a device's value/state is changed

  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
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
  static st::PS_Water_NPN sensor1(F("water1"), 1800 /*get data interval in seconds (30 min) */, 2 /* interval before and after getting data to turn on/off sensor in seconds*/, 20 /*offset in seconds*/, PIN_WATER_1, PIN_WATER_NPN_1, 200 /*limit*/);
  //static st::PS_Water_NPN sensor1(F("water1"), 10 /*get data interval in seconds (30 min) */, 2 /* interval before and after getting data to turn on/off sensor in seconds*/, 20 /*offset in seconds*/, PIN_WATER_1, PIN_WATER_NPN_1, 200 /*limit*/);
  
  //*****************************************************************************
  //  Configure debug print output from each main class
  //  -Note: Set these to "false" if using Hardware Serial on pins 0 & 1
  //         to prevent communication conflicts with the ST Shield communications
  //*****************************************************************************
  st::Everything::debug = true;
  st::Device::debug = true;
  st::PollingSensor::debug = true;
  st::PS_Water_NPN::debug = true;

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
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  st::Everything::run();
}
