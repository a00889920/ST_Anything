//******************************************************************************************
//  File: PS_Water_NPN.cpp
//  Authors: a00889920
//
//  Summary:  PS_Water is a class which implements both the SmartThings "Water Sensor" device capability.
//			  It inherits from the st::PollingSensor class.  The current version uses an analog input to measure the
//			  presence of water using an inexpensive water sensor.
//
//			  Create an instance of this class in your sketch's global variable section
//			  For Example:  st::PS_Water sensor3(F("water1"), 60, 6, PIN_WATER, 200, false);
//
//			  st::PS_Water() constructor requires the following arguments
//				- String &name - REQUIRED - the name of the object - must match the Groovy ST_Anything DeviceType tile name
//				- long interval - REQUIRED - the polling interval in seconds
//				- long offset - REQUIRED - the polling interval offset in seconds - used to prevent all polling sensors from executing at the same time
//				- byte pin - REQUIRED - the Arduino Pin to be used as an analog input
//				- int limit - OPTIONAL - the alarm limit to compare analog pin's reading to, above which the sensor reports "wet" instead of "dry", default = 100
//				- bool invertLogic - OPTIONAL - if set to true, will invert the comparison against target from < to >, default = false
//
//			  This class supports receiving configuration data from the SmartThings cloud via the ST App.  A user preference
//			  can be configured in your phone's ST App, and then the "Configure" tile will send the data for all sensors to
//			  the ST Shield.  For PollingSensors, this data is handled in the beSMart() function.
//
//			  TODO:  Determine a method to persist the ST Cloud's Polling Interval data
//
//
//      		0-10 Saturated Soil. Occurs for a day or two after irrigation
// 				10-20 Soil is adequately wet (except coarse sands which are drying out at this range)
// 				30-60 Usual range to irrigate or water (except heavy clay soils).
// 				60-100 Usual range to irrigate heavy clay soils
// 				100-200 Soil is becoming dangerously dry for maximum production. Proceed with caution.
//
//
//
//      http://www.homautomation.org/2014/06/20/measure-soil-moisture-with-arduino-gardening/
//      This uses an NPN transistor to only power the sensor when in use to help prolong the life of the sensor,
//      when current cross over the sensor it can cause corrosion across the probes.
//
//      NPN works as a switch to power up the sensor, then we take the readings and turn it off.
//
//      https://learn.sparkfun.com/tutorials/transistors/applications-i-switches
//      https://www.arduino.cc/en/Tutorial/TransistorMotorControl
//
//
//
// GND  ---- (Collector)  NPN (Emitter) -------- GND Sensor  ----------Soil Moisture Sensor  --------------------  A0 (Analog)
//                         | (Base)                                                |
//                         |                                                       |
//                  1K Ohm resistor                                                |
//                         |                                                       |
//                         |                                                       |
//                   Any Ditial pin                                             Vcc Sensor
//                       PIN 7
//
//
//
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2019-11-17  a00889920      Original Creation
//
//
//******************************************************************************************

#include "PS_Water_NPN.h"

#include "Constants.h"
#include "Everything.h"

namespace st
{
//private

//public
//constructor - called in your sketch's global variable declaration section
PS_Water_NPN::PS_Water_NPN(const __FlashStringHelper *name, unsigned int interval, unsigned int intervalPNP, int offset, byte analogInputPin, byte npnDigitalPin, int limit, bool invertLogic) : PollingSensorExtended(name, intervalPNP, interval, intervalPNP, offset),
																																															 m_nSensorValue(0),
																																															 m_nSensorLimit(limit),
																																															 m_binvertLogic(invertLogic)
{
	setPin(analogInputPin);
	setNPNPin(npnDigitalPin);
}

//destructor
PS_Water_NPN::~PS_Water_NPN()
{
}

//SmartThings Shield data handler (receives configuration data from ST - polling interval, and adjusts on the fly)
void PS_Water_NPN::beSmart(const String &str)
{
	String s = str.substring(str.indexOf(' ') + 1);

	if (s.toInt() != 0)
	{
		st::PollingSensorExtended::setInterval(s.toInt() * 1000);

		// TODO: Update server side to be able to configure these other intervals, for now, hardcoding them to 1 sec or 1000 miliseconds
		st::PollingSensorExtended::setPreInterval(1000);
		st::PollingSensorExtended::setPostInterval(1000);

		if (st::PollingSensorExtended::debug)
		{
			Serial.print(F("PS_Water_NPN::beSmart set polling interval to "));
			Serial.println(s.toInt());
		}
	}
	else
	{
		if (st::PollingSensorExtended::debug)
		{
			Serial.print(F("PS_Water_NPN::beSmart cannot convert "));
			Serial.print(s);
			Serial.println(F(" to an Integer."));
		}
	}
}

void PS_Water_NPN::preGetData()
{
	// power the sensor
	digitalWrite(m_nDigitalNPNPin, HIGH);

	if (st::PollingSensorExtended::debug)
	{
		Serial.println(F("PS_Water_NPN::Turning NPN on"));
	}
}

//function to get data from sensor and queue results for transfer to ST Cloud
void PS_Water_NPN::getData()
{
	if (st::PollingSensorExtended::debug)
	{
		Serial.println(F("PS_Water_NPN::Getting Data"));
	}
	
	int m_nSensorValue = analogRead(m_nAnalogInputPin);

	if (st::PollingSensorExtended::debug)
	{
		Serial.print(F("PS_Water_NPN::Analog Pin value is "));
		Serial.print(m_nSensorValue);
		Serial.print(F(" vs limit of "));
		Serial.println(m_nSensorLimit);
	}

	Everything::sendSmartString(getName() + " " + m_nSensorValue);
	
	//compare the sensor's value is against the limit to determine whether to send "dry" versus "wet".
	if (m_binvertLogic)
	{
		Everything::sendSmartString(getName() + (m_nSensorValue > m_nSensorLimit ? F(" dry") : F(" wet")));
	}
	else
	{
		Everything::sendSmartString(getName() + (m_nSensorValue < m_nSensorLimit ? F(" dry") : F(" wet")));
	}
}

void PS_Water_NPN::postGetData()
{
	//stop power to sensor
	digitalWrite(m_nDigitalNPNPin, LOW);

	if (st::PollingSensorExtended::debug)
	{
		Serial.println(F("PS_Water_NPN::Turning NPN off"));
	}
}

void PS_Water_NPN::setPin(byte pin)
{
	m_nAnalogInputPin = pin;
}

void PS_Water_NPN::setNPNPin(byte pin)
{
	m_nDigitalNPNPin = pin;
	pinMode(m_nDigitalNPNPin, OUTPUT);
}
} // namespace st
