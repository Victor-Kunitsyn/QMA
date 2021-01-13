#include "stdafx.h"
#include "DebugDevice.h"

#include <time.h>


CDebugSensor::CDebugSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
						   int id, int request_threshold, const std::string& name) : CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), 
                                                                                     CRemoteSensor(), 
                                                                                     is_activated(false), 
                                                                                     sensor_state(DEACTIVATED)
{
    srand( (unsigned)time( NULL ) );
}

CDebugSensor::~CDebugSensor()
{
}

ERemoteDeviceResponse CDebugSensor::Check_internal() const
{
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugSensor::Activate_internal() const
{
    if ( !is_activated )
	{
		is_activated = true;
		sensor_state = ACTIVATED;
	}
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugSensor::DeActivate_internal() const
{
    is_activated = false;
    sensor_state = DEACTIVATED;
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugSensor::IsActivated_internal() const
{
    return ( is_activated ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CDebugSensor::GetSensorState_internal(ESensorState& state) const
{
    /*int RANGE_MIN = 0;
    int RANGE_MAX = 100;
    int random_value = (((double) rand() /(double) RAND_MAX) * RANGE_MAX + RANGE_MIN);

    ESensorStateEnum sensor_state_norm;

    if ( (int(random_value / 40.)) * 40 == random_value ) //emulating sensor alarm
        sensor_state_norm = ALARMED;
    else
        sensor_state_norm = ACTIVATED;

    if ( random_value == 88 ) //emulating sensor reset
    {
        ::Sleep(1000);
        is_activated = false;
        sensor_state_norm = DEACTIVATED;
    }
    else
    {
        if ( (int(random_value / 50.)) * 50 == random_value ) //emulating no response
            return NO_RESPONSE;
    }

    sensor_state = EncodeSensorState(sensor_state_norm, random_value);*/

    state = sensor_state;
    
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugSensor::SetSensorState_internal(ESensorState state)
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
    
    if ( state != DEACTIVATED )
        ret = Activate();
	
	sensor_state = state;
    return ret;
}

/////////////////////////////////////////////////////////////////

CDebugExecuter::CDebugExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
							   int id, int request_threshold, const std::string& name) : CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), 
                                                                                         CRemoteExecuter(),
                                                                                         is_activated(false)
{
}

CDebugExecuter::~CDebugExecuter()
{
}

ERemoteDeviceResponse CDebugExecuter::Check_internal() const
{
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugExecuter::Activate_internal() const
{
    if ( !is_activated )
		is_activated = true;
    
	return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugExecuter::DeActivate_internal() const
{
    is_activated = false;
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CDebugExecuter::IsActivated_internal() const
{
    return ( is_activated ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CDebugExecuter::DoAction_internal(const void* param /*= NULL*/) const
{
    int RANGE_MIN = 0;
    int RANGE_MAX = 100;
    int random_value = (((double) rand() /(double) RAND_MAX) * RANGE_MAX + RANGE_MIN);

    if ( random_value == 88 ) //emulating no response
        return NO_RESPONSE;

    if ( is_activated )
	{
        ::Sleep(100);
		std::string msg = "Execution of: " + GetName();
		//::MessageBox(NULL, msg.c_str(), "INFO", MB_OK);
		::Beep(333, 333);
		return TRUE_RESPONSE;
	}
	else
		return FALSE_RESPONSE;
}
