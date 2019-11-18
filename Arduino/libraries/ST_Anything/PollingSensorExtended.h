//******************************************************************************************
//  File: PollingSensorExtended.cpp
//  Authors: a00889920
//
//  Summary:  st::PollingSensor is a generic class which inherits from st::Sensor.  This is the
//			  parent class for the st::PS_Illuminace, st::PS_Water, and PS_TemperatureHumidity classes.
//
//			  In general, this file should not need to be modified.
//
//			  st::PollingSensor() constructor requires the following arguments
//				- String &name - REQUIRED - the name of the object - must match the Groovy ST_Anything DeviceType tile name
//				- long interval - REQUIRED - the polling interval in seconds
//				- long offset - REQUIRED - the polling interval offset in seconds - used to prevent all polling sensors from executing at the same time
//
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2015-01-03  a00889920      Original Creation
//
//
//******************************************************************************************

#ifndef ST_POLLINGSENSOREXTENDED_H
#define ST_POLLINGSENSOREXTENDED_H

#include "Sensor.h"

namespace st
{
class PollingSensorExtended : public Sensor
{
private:
	unsigned long m_nPreviousTime; //in milliseconds - time of last poll
	long m_nDeltaTime;			   //in milliseconds - elapsed time since last poll
	long m_nIntervalPreGetData;	//in milliseconds - polling interval before getting sensor data
	long m_nIntervalGetData;	   //in milliseconds - polling interval for the sensor
	long m_nIntervalPostGetData;   //in milliseconds - polling interval after getting sensor data
	long m_nOffset;				   //in milliseconds - offset to prevent all Polling sensors from running at the same time

	bool m_bState_PreGetData; //boolean flag to indicate which state polling sensor is in
	bool m_bState_GetData;	//boolean flag to indicate which state polling sensor is in
	bool m_bState_PreGetData; //boolean flag to indicate which state polling sensor is in

	virtual bool checkInterval(); //returns true and resets m_nDeltaTime if m_nInterval has been reached

public:
	//constructor
	PollingSensorExtended(const __FlashStringHelper *name, long preInterval, long interval, long postInterval, long offset = 0);

	//destructor
	virtual ~PollingSensorExtended();

	//initialization function
	virtual void init();

	//called periodically by Everything class to ensure ST Cloud is kept consistent with the state of each Device subclass object
	virtual void refresh();

	//update function
	virtual void update();

	//function to get data from sensor and queue results for transfer to ST Cloud
	virtual void preGetData();

	//function to get data from sensor and queue results for transfer to ST Cloud
	virtual void getData();

	//function to get data from sensor and queue results for transfer to ST Cloud
	virtual void postGetData();

	//gets
	virtual void offset(long os) { m_nOffset = os; } //offset the delta time from its current value

	//sets
	virtual void setPreInterval(long interval) { m_nIntervalPreGetData = interval; }
	virtual void setInterval(long interval) { m_nIntervalGetData = interval; }
	virtual void setPostInterval(long interval) { m_nIntervalPostGetData = interval; }

	//debug flag to determine if debug print statements are executed (set value in your sketch)
	static bool debug;
};
} // namespace st

#endif