//******************************************************************************************
//  File: PollingSensorExtended.cpp
//  Authors: a00889920
//
//  Summary:  st::PollingSensorExtended is a generic class which inherits from st::PollingSensor which inherits from st::Sensor.  This is the
//			  parent class for the st::PS_Water_PNP.
//
//			  This class uses a state machine to execute actions before and after getting data, useful for when you need to power up/down the
//			  sensor before and after reading data,
//
//			  In general, this file should not need to be modified.
//
//			  st::PollingSensorExtended() constructor requires the following arguments
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

#include "PollingSensorExtended.h"

#include "Constants.h"
#include "Everything.h"

namespace st
{
//private
bool PollingSensorExtended::checkInterval()
{
	//check for time overflow
	if (millis() < m_nPreviousTime)
	{
		if (debug)
		{
			Serial.println(F("PollingSensorExtended: millis() Overflow handled"));
		}

		m_nPreviousTime = 0;
	}

	if (m_nPreviousTime == 0) //eliminates problem of there being a delay before first update() call
	{
		m_nPreviousTime = millis();
	}

	//calculate new delta time
	m_nDeltaTime += (millis() - m_nPreviousTime) - m_nOffset;
	m_nOffset = 0;
	m_nPreviousTime = millis();

	//determine interval has passed
	if (m_bState_PreGetData && m_nDeltaTime >= (m_nIntervalPreGetData - m_nIntervalGetData))
	{
		// PreGetData
		return true;
	}
	else if (m_bState_GetData && m_nDeltaTime >= m_nIntervalGetData)
	{
		// GetData
		return true;
	}
	else if (m_bState_PostGetData && m_nDeltaTime >= (m_nIntervalPreGetData + m_nIntervalPostGetData))
	{
		// PostGetData
		m_nDeltaTime = 0;
		return true;
	}
	else
	{
		return false;
	}
}

//public
//constructor
PollingSensorExtended::PollingSensorExtended(const __FlashStringHelper *name, long preInterval, long interval, long postInterval, long offset) : Sensor(name),
																																				 m_nPreviousTime(0),
																																				 m_nDeltaTime(0),
																																				 m_nIntervalPreGetData(preInterval * 1000),
																																				 m_nIntervalGetData(interval * 1000),
																																				 m_nIntervalPostGetData(postInterval * 1000),
																																				 m_nOffset(offset * 1000),
																																				 m_bState_PreGetData(true),
																																				 m_bState_GetData(false),
																																				 m_bState_PostGetData(false)
{
}

//destructor
PollingSensorExtended::~PollingSensorExtended()
{
}

void PollingSensorExtended::init()
{
	// getData();
}

void PollingSensorExtended::refresh()
{
	// getData();
}

void PollingSensorExtended::update()
{
	if (checkInterval())
	{
		if (m_bState_PreGetData)
		{
			preGetData();

			m_bState_PreGetData = false;
			m_bState_GetData = true;
		}
		else if (m_bState_GetData)
		{
			getData();

			m_bState_GetData = false;
			m_bState_PostGetData = true;
		}
		else if (m_bState_PostGetData)
		{
			postGetData();

			m_bState_PostGetData = false;
			m_bState_PreGetData = true;
		}
		else
		{
			if (debug)
			{
				Serial.println(F("PollingSensorExtended: update running with an invalid state"));
			}
		}
	}
}

void PollingSensorExtended::preGetData()
{
	if (debug)
	{
		Everything::sendSmartString(getName() + F(" triggered preGetData"));
	}
}

void PollingSensorExtended::getData()
{
	if (debug)
	{
		Everything::sendSmartString(getName() + F(" triggered getData"));
	}
}

void PollingSensorExtended::postGetData()
{
	if (debug)
	{
		Everything::sendSmartString(getName() + F(" triggered postGetData"));
	}
}

//debug flag to determine if debug print statements are executed (set value in your sketch)
bool PollingSensorExtended::debug = false;
} // namespace st