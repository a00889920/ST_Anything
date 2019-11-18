//******************************************************************************************
//  File: PS_Water_PNP.h
//  Authors: a00889920
//
//  Summary:  PS_Water_NPN is a class which implements both the SmartThings "Water Sensor" device capability.
//			  It inherits from the st::PS_Water class which inherits from st::PollingSensor class.  The current version uses an analog input to measure the
//			  presence of water using an inexpensive water sensor.
//
//			  Create an instance of this class in your sketch's global variable section
//			  For Example:  st::PS_Water_NPN sensor3(F("water1"), 60, 6, PIN_WATER, 200, false);
//
//			  st::PS_Water_PNP() constructor requires the following arguments
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
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2019-11-18  a00889920      Original Creation
//
//
//******************************************************************************************

#ifndef ST_PS_WATER_NPN_H
#define ST_PS_WATER_NPN_H

#include "PollingSensorExtended.h"

namespace st
{
class PS_Water_NPN : public PollingSensorExtended
{
private:
	byte m_nAnalogInputPin; //analog pin connected to the water sensor
	byte m_nDigitalNPNPin;  //digital pin connected to the NPN
	int m_nSensorValue;		//current sensor value
	int m_nSensorLimit;		//alarm limit
	bool m_binvertLogic;	//if false use <, if true use > for comparison of AI value versus limit

public:
	//constructor - called in your sketch's global variable declaration section
	PS_Water_NPN(const __FlashStringHelper *name, unsigned int interval, unsigned int intervalPNP, int offset, byte analogInputPin, byte npnDigitalPin, int limit = 100, bool invertLogic = false);

	//destructor
	virtual ~PS_Water_NPN();

	//SmartThings Shield data handler (receives configuration data from ST - polling interval, and adjusts on the fly)
	virtual void beSmart(const String &str);

	//function to get data from sensor and queue results for transfer to ST Cloud
	virtual void getData();

	//gets
	inline byte getPin() const { return m_nAnalogInputPin; }
	inline byte getSensorValue() const { return m_nSensorValue; }

	//sets
	void setPin(byte pin);
	void setNPNPin(byte pin);
};
} // namespace st

#endif