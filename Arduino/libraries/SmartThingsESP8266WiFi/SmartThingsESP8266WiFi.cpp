//*******************************************************************************
//	SmartThings NodeMCU ESP8266 Wifi Library
//
//	License
//	(C) Copyright 2017 Dan Ogorchock
//
//	History
//	2017-02-10  Dan Ogorchock  Created
//  2017-12-29  Dan Ogorchock  Added WiFi.RSSI() data collection
//  2018-01-06  Dan Ogorchock  Simplified the MAC address printout to prevent confusion
//  2018-01-06  Dan Ogorchock  Added OTA update capability
//  2018-02-03  Dan Ogorchock  Support for Hubitat
//  2018-12-10  Dan Ogorchock  Add user selectable host name (repurposing the old shieldType variable)
//  2019-06-03  Dan Ogorchock  Changed to wait on st_client.available() instead of st_client.connected()
//  2019-06-25  Dan Ogorchock  Fix default hostname to not use underscore character
//  2020-08-22  a00889920	   Added power savings tricks when running on battery for ESP8266
//  2020-08-22  a00889920	   Moved most serial.println to only be displayed when _isDebugEnabled is true
//  2020-08-24  a00889920      Adding suuport for device to request OTA updates for devices that sleep most of the time
//*******************************************************************************

#include "SmartThingsESP8266WiFi.h"

namespace st
{
	//*******************************************************************************
	// SmartThingsESP8266WiFI Constructor - Static IP
	//*******************************************************************************
	SmartThingsESP8266WiFi::SmartThingsESP8266WiFi(String ssid, String password, IPAddress localIP, IPAddress localGateway, IPAddress localSubnetMask, IPAddress localDNSServer, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType, bool enableDebug, int transmitInterval) :
		SmartThingsEthernet(localIP, localGateway, localSubnetMask, localDNSServer, serverPort, hubIP, hubPort, callout, shieldType, enableDebug, transmitInterval, false),
		st_server(serverPort)
	{
		ssid.toCharArray(st_ssid, sizeof(st_ssid));
		password.toCharArray(st_password, sizeof(st_password));
	}

	//*******************************************************************************
	// SmartThingsESP8266WiFI Constructor for Battery powered devices - Static IP
	//*******************************************************************************
	SmartThingsESP8266WiFi::SmartThingsESP8266WiFi(String ssid, String password, IPAddress localIP, IPAddress localGateway, IPAddress localSubnetMask, IPAddress localDNSServer, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType, bool enableDebug, int transmitInterval, bool runningOnBattery, bool enableNetworkPersistance, bool enableOnDemandOTAUpdated, int firmwareVersion, String firmwareServerUrl) :
		SmartThingsEthernet(localIP, localGateway, localSubnetMask, localDNSServer, serverPort, hubIP, hubPort, callout, shieldType, enableDebug, transmitInterval, false),
		st_server(serverPort),
		m_runningOnBattery(runningOnBattery),
		m_enableNetworkPersistance(enableNetworkPersistance),
		m_enableOnDemandOTAUpdated(enableOnDemandOTAUpdated),
		FW_VERSION(firmwareVersion),
		FW_ServerUrl(firmwareServerUrl)
	{
		startTimeMilis = millis();
		if (m_runningOnBattery)
		{
			if (_isDebugEnabled) Serial.println("------------ preInit RUNNING_ON_BATTERY_DISABLE_WIFI_WHEN_WAKING_UP");
		
			// Disabling WiFi when waking up
			//
			// https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-2/
			//
			// As the WiFi radio is on when the ESP wakes up, we wake up with 70 mA current even if we’re not using the WiFi yet.
			// To try to reduce this, let’s switch off the WiFi radio at the beginning of the setup() function, keep it off while
			// we’re reading the sensors, and switch it back on when we are ready to send the results to the server.
			//
			// From the experiments, it was found that both WiFi.mode() and WiFi.forceSleepBegin() were required in order to
			// switch off the radio. The forceSleepBegin() call will set the flags and modes necessary, but the radio will not
			// actually switch off until control returns to the ESP ROM. To do that we’re adding a delay( 1 ),
			// but I suppose a yield() would work as well.
			WiFi.mode(WIFI_OFF);
  			WiFi.forceSleepBegin();
  			yield();
		}

		ssid.toCharArray(st_ssid, sizeof(st_ssid));
		password.toCharArray(st_password, sizeof(st_password));
	}

	//*******************************************************************************
	// SmartThingsESP8266WiFI Constructor - DHCP
	//*******************************************************************************
	SmartThingsESP8266WiFi::SmartThingsESP8266WiFi(String ssid, String password, uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType, bool enableDebug, int transmitInterval) :
		SmartThingsEthernet(serverPort, hubIP, hubPort, callout, shieldType, enableDebug, transmitInterval, true),
		st_server(serverPort)
	{
		ssid.toCharArray(st_ssid, sizeof(st_ssid));
		password.toCharArray(st_password, sizeof(st_password));
	}

	//*******************************************************************************
	// SmartThingsESP8266WiFI Constructor - DHCP
	//*******************************************************************************
	SmartThingsESP8266WiFi::SmartThingsESP8266WiFi(uint16_t serverPort, IPAddress hubIP, uint16_t hubPort, SmartThingsCallout_t *callout, String shieldType, bool enableDebug, int transmitInterval) :
		SmartThingsEthernet(serverPort, hubIP, hubPort, callout, shieldType, enableDebug, transmitInterval, true),
		st_server(serverPort)
	{
		st_preExistingConnection = true;
	}

	//*****************************************************************************
	//SmartThingsESP8266WiFI::~SmartThingsESP8266WiFI()
	//*****************************************************************************
	SmartThingsESP8266WiFi::~SmartThingsESP8266WiFi()
	{

	}

	//*******************************************************************************
	/// Initialize SmartThingsESP8266WiFI Library
	//*******************************************************************************
	void SmartThingsESP8266WiFi::init(void)
	{
		if (!st_preExistingConnection) {
			if (_isDebugEnabled){
				Serial.println(F(""));
				Serial.println(F("Initializing ESP8266 WiFi network.  Please be patient..."));
			}

			if(m_runningOnBattery)
			{
				if (_isDebugEnabled) Serial.println("------------ init RUNNING_ON_BATTERY_DISABLE_WIFI_WHEN_WAKING_UP");
				// https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-2/
				// Just before the calls to establish the WiFi connection, we switch the radio back on:
				// forceSleepWake() will set the correct flags and modes, but the change will not take effect 
  				// until control returns to the ESP ROM, so we add a delay( 1 ) call here as well.				
				WiFi.forceSleepWake();
				yield();
			}
			
			WiFi.mode(WIFI_STA);

			if (st_DHCP == false)
			{
				if(m_enableNetworkPersistance)
				{
					if (_isDebugEnabled) Serial.println("------------ RUNNING_ON_BATTERY_DISABLING_NETWORK_PERSISTANCE");
					// Disabling network persistence
					//
					// https://www.bakke.online/index.php/2017/05/22/reducing-wifi-power-consumption-on-esp8266-part-3/
					//
					// The ESP8266 will persist the network connection information to flash,
					// and then read this back when it next starts the WiFi function. It does this every time, and 
					// from experiments, it was found that this takes at least 1.2 seconds. There are cases where
					// the WiFi function would crash the chip, and the WiFi would never connect.
					//
					// The chip also does this even when you pass connection information to WiFi.begin(), i.e. even in the case below:
					// WiFi.begin( WLAN_SSID, WLAN_PASSWD );
					// This will actually load the connection information from flash, promptly ignore it and use the 
					// values you specify instead, connect to the WiFi and then finally write your values back to flash.
					//
					// This starts wearing out the flash memory after a while. Exactly how quickly or slowly will depend on
					// the quality of the flash memory connected to your ESP8266 chip.
					//
					// The good news is that we can disable or enable this persistence by calling WiFi.persistent().
					WiFi.persistent(false);
				}

				WiFi.config(st_localIP, st_localGateway, st_localSubnetMask, st_localDNSServer);
			}

			if(m_enableNetworkPersistance)
			{
				if (_isDebugEnabled) Serial.println("------------ RUNNING_ON_BATTERY_WIFI_QUICK_CONNECT");
			
				// https://www.bakke.online/index.php/2017/06/24/esp8266-wifi-power-reduction-avoiding-network-scan/

				// Introducing the RTC and its memory
				// When the ESP8266 goes into deep sleep, a part of the chip called the RTC remains awake.  
				// Its power consumption is extremely low, so the ESP8266 does not use much power when in deep sleep.
				// This is the component responsible for generating the wakeup signal when our deep sleep times out.
				// That’s not the only benefit of the RTC, though.  It also has its own memory, which we can actually
				// read from and write to from the main ESP8266 processor.
				//
				// WiFi quick connect
				// The WiFi.begin() method has an overload which accepts a WiFi channel number and a BSSID.  By passing
				// these, the ESP8266 will attempt to connect to a specific WiFi access point on the channel given, 
				// thereby eliminating the need to scan for an access point advertising the requested network.
				//
				// How much time this will save depends on your network infrastructure, how many WiFi networks are active,
				// how busy the network is and the signal strength.  It WILL save time, though, time in which the WiFi radio
				// is active and drawing power from the batteries.
				//
				// Only problem is: How is the ESP8266 going to know which channel and BSSID to use when it wakes up, without doing a scan?
				//
				// That’s where we can use the memory in the RTC.  As the RTC remains powered during deep sleep, anything we
				// write into its memory will still be there when the ESP wakes up again.
				//
				// So, when the ESP8266 wakes up, we’ll read the RTC memory and check if we have a valid configuration to use.
				// If we do, we’ll pass the additional parameters to WiFi.begin().  If we don’t, we’ll just connect as normal.
				// As soon as the connection has been established, we can write the channel and BSSID into the RTC memory, ready for next time.
				//
				// Reading and writing this memory is explained in the RTCUserMemory example in the Arduino board support package,
				// and the code below is based on that example

				// Try to read WiFi settings from RTC memory
				bool rtcValid = false;
				if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
					// Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
					uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
					if( crc == rtcData.crc32 ) {
						rtcValid = true;
					}
				}

				if( rtcValid ) {
					// The RTC data was good, make a quick connection
					if (_isDebugEnabled) Serial.println(F("Using RTC data to make a quick connection."));
					WiFi.begin( st_ssid, st_password, rtcData.channel, rtcData.bssid, true );
				} else {
					// The RTC data was not valid, so make a regular connection
					if (_isDebugEnabled) Serial.println(F("RTC data is not valid, making a regular connection."));
					WiFi.begin( st_ssid, st_password );
				}
			} else {
				// attempt to connect to WiFi network
				WiFi.begin(st_ssid, st_password);
			}

			if (_isDebugEnabled) {
				Serial.print(F("Attempting to connect to WPA SSID: "));
			    Serial.println(st_ssid);
			}
		}

		if(m_enableNetworkPersistance)
		{
			// The loop waiting for the WiFi connection to be established becomes a little more complicated
			// as we’ll have to consider the possibility that the WiFi access point has changed channels,
			// or that the access point itself has been changed.  So, if we haven’t established a connection
			// after a certain number of loops, we’ll reset the WiFi and try again with a normal connection. 
			// If, after 30 seconds, we still don’t have a connection we’ll go back to sleep and try again the
			// next time we wake up.  After all, it could be that the WiFi network is temporarily unavailable
			// and we don’t want to stay awake until it comes back. Better to go back to sleep and hope
			// things are better in the morning…

			int retries = 0;
			int wifiStatus = WiFi.status();
			while( wifiStatus != WL_CONNECTED ) {
				retries++;

				if( retries == 100 ) {
					// Quick connect is not working, reset WiFi and try regular connection
					if (_isDebugEnabled)  Serial.println(" Reset WiFi and try regular connection");
					WiFi.disconnect();
					delay( 10 );
					WiFi.forceSleepBegin();
					delay( 10 );
					WiFi.forceSleepWake();
					delay( 10 );
					WiFi.begin( st_ssid, st_password );
				}

				if( retries == 600 ) {
					// Giving up after 30 seconds and going back to sleep
					if (_isDebugEnabled)  Serial.println(" Giving up, go to sleep");
					WiFi.mode( WIFI_OFF );
					deepSleep(30 * 1000000);
					return; // Not expecting this to be called, the previous call will never return.
				}

				delay( 50 );
				wifiStatus = WiFi.status();
				if (_isDebugEnabled)  {
					Serial.print(" Retry # ");
					Serial.println(retries);
				}
			}

			// Once the WiFi is connected, we can get the channel and BSSID and stuff it into the
			// RTC memory, ready for the next time we wake up.

			// TODO: Only write information into RTC Memory if it has changed

			// Write current connection info back to RTC
			
			byte macAddressByte[6];
			WiFi.macAddress(macAddressByte);

			if (_isDebugEnabled)  {
				byte bssidddddd[6];  
				memcpy( bssidddddd, WiFi.BSSID(), 6 );  
				Serial.print("----------BSSID: ");
				Serial.print(bssidddddd[0],HEX);
				Serial.print(":");
				Serial.print(bssidddddd[1],HEX);
				Serial.print(":");
				Serial.print(bssidddddd[2],HEX);
				Serial.print(":");
				Serial.print(bssidddddd[3],HEX);
				Serial.print(":");
				Serial.print(bssidddddd[4],HEX);
				Serial.print(":");
				Serial.println(bssidddddd[5],HEX);
				

				byte mac[6];
				WiFi.macAddress(mac);
				Serial.print("------------MAC: ");
				Serial.print(mac[0],HEX);
				Serial.print(":");
				Serial.print(mac[1],HEX);
				Serial.print(":");
				Serial.print(mac[2],HEX);
				Serial.print(":");
				Serial.print(mac[3],HEX);
				Serial.print(":");
				Serial.print(mac[4],HEX);
				Serial.print(":");
				Serial.println(mac[5],HEX);
			}

			if (memcmp(macAddressByte, rtcData.mac, 6) == 0) {
				if (_isDebugEnabled) Serial.println(" mac has changed");
				memcpy( rtcData.mac, macAddressByte, 6 ); // Copy 6 bytes of MAC address
			}
			if (memcmp(rtcData.bssid, WiFi.BSSID(), 6)) {
				if (_isDebugEnabled) Serial.println(" bssid has changed");
				memcpy( rtcData.bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
			}
			if (rtcData.channel != WiFi.channel()) {
				if (_isDebugEnabled) Serial.println(" wfi channel has changed");
				rtcData.channel = WiFi.channel();
			} 
		
			uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
			if( crc != rtcData.crc32 ) {
				rtcData.crc32 = crc;
				if (_isDebugEnabled)  Serial.println(" RTC memory has changed, updating it");
				ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );		
			}
		} else {
			while (WiFi.status() != WL_CONNECTED) {
				Serial.print(F("."));
				delay(500);	// wait for connection:
			}
		}

		st_server.begin();
		String strMAC(WiFi.macAddress());

		if (_isDebugEnabled) {
			Serial.println();

			Serial.println(F(""));
			Serial.println(F("Enter the following three lines of data into ST App on your phone!"));
			Serial.print(F("localIP = "));
			Serial.println(WiFi.localIP());
			Serial.print(F("serverPort = "));
			Serial.println(st_serverPort);
			Serial.print(F("MAC Address = "));
			strMAC.replace(":", "");
			Serial.println(strMAC);
			Serial.println(F(""));
			Serial.print(F("SSID = "));
			Serial.println(st_ssid);
			Serial.print(F("PASSWORD = "));
			Serial.println(st_password);
			Serial.print(F("hubIP = "));
			Serial.println(st_hubIP);
			Serial.print(F("hubPort = "));
			Serial.println(st_hubPort);
			Serial.print(F("RSSI = "));
			Serial.println(WiFi.RSSI());
		}

		if (_shieldType == "ESP8266Wifi") {
			String("ESP8266-" + strMAC).toCharArray(st_devicename, sizeof(st_devicename));
		}
		else {
			_shieldType.toCharArray(st_devicename, sizeof(st_devicename));
		}

		if (_isDebugEnabled) {
			Serial.print(F("hostName = "));
			Serial.println(st_devicename);
		}

		WiFi.hostname(st_devicename);

		if (_isDebugEnabled) {
			Serial.println(F(""));
			Serial.println(F("SmartThingsESP8266WiFI: Intialized"));
			Serial.println(F(""));

			//Turn off Wirelss Access Point
			Serial.println(F("Disabling ESP8266 WiFi Access Point"));
			Serial.println(F(""));
		}

		RSSIsendInterval = 5000;
		previousMillis = millis() - RSSIsendInterval;

		if (m_enableOnDemandOTAUpdated)
		{
			checkForOnDemandOTAUpdates();
		}
		else
		{
			// Setup OTA Updates

			// Port defaults to 8266
			// ArduinoOTA.setPort(8266);

			// Hostname defaults to esp8266-[ChipID]
			ArduinoOTA.setHostname(st_devicename);

			// No authentication by default
			//ArduinoOTA.setPassword((const char*)"123");

			ArduinoOTA.onStart([]() {
				Serial.println("Start");
			});
			ArduinoOTA.onEnd([]() {
				Serial.println("\nEnd");
			});
			ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
				Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			});
			ArduinoOTA.onError([](ota_error_t error) {
				Serial.printf("Error[%u]: ", error);
				if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
				else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
				else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
				else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
				else if (error == OTA_END_ERROR) Serial.println("End Failed");
			});
			ArduinoOTA.begin();

			if (_isDebugEnabled) {
				Serial.println("ArduinoOTA Ready");
				Serial.print("IP address: ");
				Serial.println(WiFi.localIP());
				Serial.print("ArduionOTA Host Name: ");
				Serial.println(ArduinoOTA.getHostname());
				Serial.println();
			}
		}
	}

	//*****************************************************************************
	// Run SmartThingsESP8266WiFI Library
	//*****************************************************************************
	void SmartThingsESP8266WiFi::run(void)
	{
		if (!m_enableOnDemandOTAUpdated)
		{
			ArduinoOTA.handle();
		}

		String readString;
		String tempString;
		String strRSSI;

		if (WiFi.isConnected() == false)
		{
			if (_isDebugEnabled)
			{
				Serial.println(F("**********************************************************"));
				Serial.println(F("**** WiFi Disconnected.  ESP8266 should auto-reconnect ***"));
				Serial.println(F("**********************************************************"));
			}

			//init();
		}
		else
		{
			if (millis() - previousMillis > RSSIsendInterval)
			{

				previousMillis = millis();

				if (RSSIsendInterval < RSSI_TX_INTERVAL)
				{
					RSSIsendInterval = RSSIsendInterval + 1000;
				}
				
				strRSSI = String("rssi ") + String(WiFi.RSSI());
				send(strRSSI);

				if (_isDebugEnabled)
				{
					Serial.println(strRSSI);
				}
			}
		}

		WiFiClient client = st_server.available();
		if (client) {
			boolean currentLineIsBlank = true;
			while (client.connected()) {
				if (client.available()) {
					char c = client.read();
					//read char by char HTTP request
					if (readString.length() < 200) {
						//store characters to string
						readString += c;
					}
					else
					{
						if (_isDebugEnabled)
						{
							Serial.println(F(""));
							Serial.println(F("SmartThings.run() - Exceeded 200 character limit"));
							Serial.println(F(""));
						}
					}
					// if you've gotten to the end of the line (received a newline
					// character) and the line is blank, the http request has ended,
					// so you can send a reply
					if (c == '\n' && currentLineIsBlank) {
						//now output HTML data header
						tempString = readString.substring(readString.indexOf('/') + 1, readString.indexOf('?'));

						if (tempString.length() > 0) {
							client.println(F("HTTP/1.1 200 OK")); //send new page
							client.println();
						}
						else {
							client.println(F("HTTP/1.1 204 No Content"));
							client.println();
							client.println();
							if (_isDebugEnabled)
							{
								Serial.println(F("No Valid Data Received"));
							}
						}
						break;
					}
					if (c == '\n') {
						// you're starting a new line
						currentLineIsBlank = true;
					}
					else if (c != '\r') {
						// you've gotten a character on the current line
						currentLineIsBlank = false;
					}
				}
			}

			delay(1);
			//stopping client
			client.stop();

			//Handle the received data after cleaning up the network connection
			if (tempString.length() > 0) {
				if (_isDebugEnabled)
				{
					Serial.print(F("Handling request from ST. tempString = "));
					Serial.println(tempString);
				}
				//Pass the message to user's SmartThings callout function
				tempString.replace("%20", " ");  //Clean up for Hubitat
				_calloutFunction(tempString);
			}

			readString = "";
			tempString = "";
		}
	}

	//*******************************************************************************
	/// Send Message out over Ethernet to the Hub
	//*******************************************************************************
	void SmartThingsESP8266WiFi::send(String message)
	{
		if (WiFi.isConnected() == false)
		{
			if (_isDebugEnabled)
			{
				Serial.println(F("**********************************************************"));
				Serial.println(F("**** WiFi Disconnected.  ESP8266 should auto-reconnect ***"));
				Serial.println(F("**********************************************************"));
			}

			//init();
		}

		//Make sure the client is stopped, to free up socket for new conenction
		st_client.stop();

		if (st_client.connect(st_hubIP, st_hubPort))
		{
			st_client.println(F("POST / HTTP/1.1"));
			st_client.print(F("HOST: "));
			st_client.print(st_hubIP);
			st_client.print(F(":"));
			st_client.println(st_hubPort);
			st_client.println(F("CONTENT-TYPE: text"));
			st_client.print(F("CONTENT-LENGTH: "));
			st_client.println(message.length());
			st_client.println();
			st_client.println(message);
		}
		else
		{
			//connection failed;
			if (_isDebugEnabled)
			{
				Serial.println(F("***********************************************************"));
				Serial.println(F("***** SmartThings.send() - Ethernet Connection Failed *****"));
				Serial.println(F("***********************************************************"));
				Serial.print(F("hubIP = "));
				Serial.print(st_hubIP);
				Serial.print(F(" "));
				Serial.print(F("hubPort = "));
				Serial.println(st_hubPort);

				Serial.println(F("***********************************************************"));
				Serial.println(F("**** WiFi Disconnected.  ESP8266 should auto-reconnect ****"));
				Serial.println(F("***********************************************************"));
			}

			//init();      //Re-Init connection to get things working again

			if (_isDebugEnabled)
			{
				Serial.println(F("***********************************************************"));
				Serial.println(F("******        Attempting to resend missed data      *******"));
				Serial.println(F("***********************************************************"));
			}


			st_client.flush();
			st_client.stop();
			if (st_client.connect(st_hubIP, st_hubPort))
			{
				st_client.println(F("POST / HTTP/1.1"));
				st_client.print(F("HOST: "));
				st_client.print(st_hubIP);
				st_client.print(F(":"));
				st_client.println(st_hubPort);
				st_client.println(F("CONTENT-TYPE: text"));
				st_client.print(F("CONTENT-LENGTH: "));
				st_client.println(message.length());
				st_client.println();
				st_client.println(message);
			}

		}

		//if (_isDebugEnabled) { Serial.println(F("WiFi.send(): Reading for reply data "));}
		// read any data returned from the POST
		//while (st_client.connected()) {
		    while (st_client.available()) {
			    char c = st_client.read(); //gets byte from ethernet buffer
				//if (_isDebugEnabled) { Serial.print(c); } //prints byte to serial monitor
			}
		//}

		delay(1);
		st_client.stop();
	}
	
	//*******************************************************************************
	/// Puts device into Deepsleep 
	//*******************************************************************************
	void SmartThingsESP8266WiFi::deepSleep(uint64_t time)
	{
		//if (_isDebugEnabled) {
			long runTime = millis() - startTimeMilis;
			Serial.println();
			Serial.print("------------ Run time:");
			Serial.println(runTime);
		//}

		// Report to device handler the run time
		String strFRunTime = String("RunTime ") + String(runTime);
		send(strFRunTime);

		if(m_runningOnBattery)
		{
			// Using WAKE_RF_DISABLED
			//
			// https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-2/
			//
			// There’s a sharp power peak as the ESP wakes up, and by the time the setup() is called,
			// we will have used 0.008 mAh already.
			// To avoid this we can go to sleep using the WAKE_RF_DISABLED flag. This configures the
			// chip to keep the radio disabled until told to enable it.
			//
			// The calls to WiFi.disconnect() and delay() are needed in order to ensure the chip goes
			// into proper deep sleep. Without them, the chip usually ends up consuming about 1.2 mA 
			// of current while sleeping. This indicates that deep sleep was not achieved and that the
			// chip is in Power Save DTIM3 mode.
			//
			// the ESP now wakes up with the radio disabled, it stays disabled until after the sensors
			// have been read and we can collect another 0.006 mAh reduction for a total of 0.024 mAh.
			WiFi.disconnect(true);
			yield();
			
			if (_isDebugEnabled) Serial.println("------------ WAKE_RF_DISABLED");
			// WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
			ESP.deepSleep(time, WAKE_RF_DISABLED);
		} else {
			ESP.deepSleep(time);
		}
	}
	
	//*******************************************************************************
	/// Calculate CRC32
	//*******************************************************************************
	uint32_t SmartThingsESP8266WiFi::calculateCRC32(const uint8_t *data, size_t length)
	{
		uint32_t crc = 0xffffffff;
		while(length--) {
			uint8_t c = *data++;
			for(uint32_t i = 0x80; i > 0; i >>= 1) {
				bool bit = crc & 0x80000000;
				if(c & i) {
					bit = !bit;
				}

				crc <<= 1;
				if(bit) {
					crc ^= 0x04c11db7;
				}
			}
		}

		return crc;
	}

	//*******************************************************************************
	/// On demand Self Updating OTA
	//*******************************************************************************
	void SmartThingsESP8266WiFi::checkForOnDemandOTAUpdates() {
		// Each device has its own MAC Address
		// The server will have one folder per device with format MACAddress
		// Inside the folder, there will be a latest.version file which contains
		// a single 32bit integer, nothing else
		// This will be used to compare current version and locate the new 
		// firmaware we need to update to.
		// NOTE: the new Sketch should set the FW_VERSION to match, otherwise
		// we will be in a cycle updating each time it boots.

		// Example: http://192.168.254.16/FirmwareOTA/0000d3fdff3f/latest.version
		// Content: 1001
		// http://192.168.254.16/FirmwareOTA/0000d3fdff3f/0000d3fdff3f-1000.bin
		// http://192.168.254.16/FirmwareOTA/0000d3fdff3f/0000d3fdff3f-1001.bin

		// Report to device handler the firmware version
		String strFWVersion = String("fwVersion ") + String(FW_VERSION);
		send(strFWVersion);

		String mac = getMAC();
		String fwURL = String( FW_ServerUrl );
		fwURL.concat( "/" );
		fwURL.concat( mac );
		String fwVersionURL = fwURL;
		fwVersionURL.concat( "/latest.version" );

		if (_isDebugEnabled) {
			Serial.println( "Checking for firmware updates." );
			Serial.print( "MAC address: " );
			Serial.println( mac );
			Serial.print( "Firmware version URL: " );
			Serial.println( fwVersionURL );
		}

		HTTPClient httpClient;
		httpClient.begin( fwVersionURL );
		int httpCode = httpClient.GET();
		if( httpCode == 200 ) {
			String newFWVersion = httpClient.getString();

			if (_isDebugEnabled) {
				Serial.print( "Current firmware version: " );
				Serial.println( FW_VERSION );
				Serial.print( "Available firmware version: " );
				Serial.println( newFWVersion );
			}

			int newVersion = newFWVersion.toInt();

			if( newVersion > FW_VERSION ) {
				if (_isDebugEnabled) Serial.println( "Preparing to update" );

				// Looking for Firmware file with format MACAddress-Version.bin
				String fwImageURL = fwURL;
				fwImageURL.concat( "/" );
				fwImageURL.concat( mac );
				fwImageURL.concat( "-" );
				fwImageURL.concat( newFWVersion );
				fwImageURL.concat( ".bin" );
				if (_isDebugEnabled) Serial.println( "Using firmware file " +  fwImageURL);
				t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

				switch(ret) {
					case HTTP_UPDATE_FAILED:
					Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
					break;

					case HTTP_UPDATE_NO_UPDATES:
					Serial.println("HTTP_UPDATE_NO_UPDATES");
					break;
				}
			}
			else {
				Serial.println( "Already on latest version" );
			}
		}
		else {
			if (_isDebugEnabled){
				Serial.print( "Firmware version check failed, got HTTP response code " );
				Serial.println( httpCode );
			}
		}
		httpClient.end();
	}


	//*******************************************************************************
	/// getMAC for OTA
	//*******************************************************************************
	String SmartThingsESP8266WiFi::getMAC()
	{
		byte mac[6];
		WiFi.macAddress(mac);
		char result[14];

		snprintf(result, sizeof(result), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		return String(result);
	}
}