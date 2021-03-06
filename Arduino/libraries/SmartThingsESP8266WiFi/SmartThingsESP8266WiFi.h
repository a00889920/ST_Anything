//*******************************************************************************
//	SmartThings Arduino ESP8266 Wifi Library
//
//	License
//	(C) Copyright 2017 Dan Ogorchock
//
//	History
//	2017-02-05  Dan Ogorchock  Created
//  2017-12-29  Dan Ogorchock  Added WiFi.RSSI() data collection
//  2018-01-06  Dan Ogorchock  Added OTA update capability
//  2018-12-10  Dan Ogorchock  Add user selectable host name (repurposing the old shieldType variable)
//  2019-05-01  Dan Ogorchock  Changed max transmit rate from every 100ms to every 
//                             500ms to prevent duplicate child devices
//  2020-08-22  a00889920	   Added power savings tricks when running on battery for ESP8266
//  2020-08-24  a00889920      Adding suuport for device to request OTA updates for devices that sleep most of the time
//  2020-09-02  a00889920      Moving OTA code to its own repo https://github.com/a00889920/OTAOnDemand_master
//*******************************************************************************

#ifndef __SMARTTHINGSESP8266WIFI_H__
#define __SMARTTHINGSESP8266WIFI_H__

#include "SmartThingsEthernet.h"

//*******************************************************************************
// Using ESP8266 WiFi
//*******************************************************************************
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <OTAOnDemand.h>

namespace st
{
	class SmartThingsESP8266WiFi: public SmartThingsEthernet
	{
	private:
		//ESP8266 WiFi Specific
		char st_ssid[50];
		char st_password[50];
		boolean st_preExistingConnection = false;
		WiFiServer st_server; //server
		WiFiClient st_client; //client
		long previousMillis;
		long RSSIsendInterval;
		char st_devicename[50];
		boolean m_runningOnBattery = false;
		boolean m_enableNetworkPersistance = false;
		
		boolean m_enableOnDemandOTAUpdated = false;
		OTAOnDemand otaOnDemand;

		long startTimeMilis;

		// The ESP8266 RTC memory is arranged into blocks of 4 bytes. The access methods read and write 4 bytes at a time,
		// so the RTC data structure should be padded to a 4-byte multiple.
		struct {
			uint32_t crc32;   	// 4 bytes
			uint8_t channel;  	// 1 byte,   5 in total
			uint8_t bssid[6]; 	// 6 bytes, 11 in total
			uint8_t mac[6]; 	// 6 bytes, 17 in total
			uint8_t padding;  	// 1 byte,  18 in total
		} rtcData;

		//*******************************************************************************
		/// Calculate CRC32
		//*******************************************************************************
		uint32_t calculateCRC32(const uint8_t *data, size_t length);

		//*******************************************************************************
		/// Performs OTA on demand update using firmware files in a local server
		//*******************************************************************************
		void checkForOnDemandOTAUpdates();

		//*******************************************************************************
		/// Gets MAC address in a format for OTA on demand updates
		//*******************************************************************************
		String getMAC();

	public:

		//*******************************************************************************
		/// @brief  SmartThings ESP8266 WiFi Constructor - Static IP
		///   @param[in] ssid - Wifi Network SSID
		///   @param[in] password - Wifi Network Password
		///   @param[in] localIP - TCP/IP Address of the Arduino
		///   @param[in] localGateway - TCP/IP Gateway Address of local LAN (your Router's LAN Address)
		///   @param[in] localSubnetMask - Subnet Mask of the Arduino
		///   @param[in] localDNSServer - DNS Server
		///   @param[in] serverPort - TCP/IP Port of the Arduino
		///   @param[in] hubIP - TCP/IP Address of the ST Hub
		///   @param[in] hubPort - TCP/IP Port of the ST Hub
		///   @param[in] callout - Set the Callout Function that is called on Msg Reception
		///   @param[in] shieldType (optional) - Set the Reported SheildType to the Server
		///   @param[in] enableDebug (optional) - Enable internal Library debug
		///   @param[in] transmitInterval (optional) - Interval for transmiting
		//*******************************************************************************
		SmartThingsESP8266WiFi(String ssid, String password, IPAddress localIP, IPAddress localGateway, IPAddress localSubnetMask, IPAddress localDNSServer, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType = "ESP8266Wifi", bool enableDebug = false, int transmitInterval = 500);

		//*******************************************************************************
		/// @brief  SmartThings ESP8266 WiFi Constructor - Static IP
		///   @param[in] ssid - Wifi Network SSID
		///   @param[in] password - Wifi Network Password
		///   @param[in] localIP - TCP/IP Address of the Arduino
		///   @param[in] localGateway - TCP/IP Gateway Address of local LAN (your Router's LAN Address)
		///   @param[in] localSubnetMask - Subnet Mask of the Arduino
		///   @param[in] localDNSServer - DNS Server
		///   @param[in] serverPort - TCP/IP Port of the Arduino
		///   @param[in] hubIP - TCP/IP Address of the ST Hub
		///   @param[in] hubPort - TCP/IP Port of the ST Hub
		///   @param[in] callout - Set the Callout Function that is called on Msg Reception
		///   @param[in] shieldType (optional) - Set the Reported SheildType to the Server
		///   @param[in] enableDebug (optional) - Enable internal Library debug
		///   @param[in] transmitInterval (optional) - Interval for transmiting
		///   @param[in] runningOnBattery (optional) - Enable battery power saving tricks
		///   @param[in] enableOnDemandOTAUpdated (optional) - Enable on demand OTA updates after init
		///   @param[in] firmwareVersion (optional) - Sketch Firmware version number used for on demand OTA
		///   @param[in] firmwareServerUrl (optional) - Server URL hosting firmware files to be used for on demand OTA
		//*******************************************************************************
		SmartThingsESP8266WiFi(String ssid, String password, IPAddress localIP, IPAddress localGateway, IPAddress localSubnetMask, IPAddress localDNSServer, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType = "ESP8266Wifi", bool enableDebug = false, int transmitInterval = 500, bool runningOnBattery = false, bool enableNetworkPersistance = false, bool enableOnDemandOTAUpdated = false, int firmwareVersion = 1, String firmwareServerUrl = "");


		//*******************************************************************************
		/// @brief  SmartThings ESP8266 WiFi Constructor - DHCP
		///   @param[in] ssid - Wifi Network SSID
		///   @param[in] password - Wifi Network Password
		///   @param[in] serverPort - TCP/IP Port of the Arduino
		///   @param[in] hubIP - TCP/IP Address of the ST Hub
		///   @param[in] hubPort - TCP/IP Port of the ST Hub
		///   @param[in] callout - Set the Callout Function that is called on Msg Reception
		///   @param[in] shieldType (optional) - Set the Reported SheildType to the Server
		///   @param[in] enableDebug (optional) - Enable internal Library debug
		//*******************************************************************************
		SmartThingsESP8266WiFi(String ssid, String password, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType = "ESP8266Wifi", bool enableDebug = false, int transmitInterval = 500);

		//*******************************************************************************
		/// @brief  SmartThings ESP8266 WiFi Constructor - Pre-existing connection
		///   @param[in] serverPort - TCP/IP Port of the Arduino
		///   @param[in] hubIP - TCP/IP Address of the ST Hub
		///   @param[in] hubPort - TCP/IP Port of the ST Hub
		///   @param[in] callout - Set the Callout Function that is called on Msg Reception
		///   @param[in] shieldType (optional) - Set the Reported SheildType to the Server
		///   @param[in] enableDebug (optional) - Enable internal Library debug
		//*******************************************************************************
		SmartThingsESP8266WiFi(uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType = "ESP8266Wifi", bool enableDebug = false, int transmitInterval = 500);

		//*******************************************************************************
		/// Destructor
		//*******************************************************************************
		~SmartThingsESP8266WiFi();

		//*******************************************************************************
		/// Initialize SmartThingsESP8266WiFI Library
		//*******************************************************************************
		virtual void init(void);

		//*******************************************************************************
		/// Run SmartThingsESP8266WiFI Library
		//*******************************************************************************
		virtual void run(void);

		//*******************************************************************************
		/// Send Message to the Hub
		//*******************************************************************************
		virtual void send(String message);

		//*******************************************************************************
		/// Puts device into Deepsleep 
		//*******************************************************************************
		virtual void deepSleep(uint64_t time);

	};
}
#endif
