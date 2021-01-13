#include "stdafx.h"
#include "ArduinoSensor.h"

CArduinoSensor::CArduinoSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
							   int id, int request_threshold, const std::string& name) : CArduinoDevice(ip, port, usr, psw, id, request_threshold, name), CRemoteSensor()
{
}

CArduinoSensor::~CArduinoSensor()
{
}

//http parameters:

//get sensor state: request ?I=id&T=S response "Sensor state" 
//set sensor state: request ?I=id&T="Sensor state" response "1" (true) or "0" (false)

ERemoteDeviceResponse CArduinoSensor::GetSensorState_internal(ESensorState& state) const
{
	CheckRequestTimeTreshold();

	device_cs.Enter();

	state = DEACTIVATED;
    
    ERemoteDeviceResponse ret = ( SendRequestToServer("T=S") ? TRUE_RESPONSE :  NO_RESPONSE );
	
	if ( ret == TRUE_RESPONSE )
	{
		if ( o_result_xml != "" )
        {
            int state_dig = atoi(o_result_xml.c_str());
            state = ESensorState((ESensorStateEnum)state_dig);
        }
        else
            ret = NO_RESPONSE;
	}
	
	device_cs.Leave();

	return ret;
}

ERemoteDeviceResponse CArduinoSensor::SetSensorState_internal(ESensorState state)
{
	CheckRequestTimeTreshold();

	device_cs.Enter();

	std::string request = "T=";
	char temp_buf[10];
    sprintf(temp_buf, "%d", int(state.GetRaw()));
	request += temp_buf;
    	
	ERemoteDeviceResponse ret = ( SendRequestToServer(request.c_str()) ? TRUE_RESPONSE :  NO_RESPONSE );
	ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
	
	device_cs.Leave();

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CArduinoSensorWithActivator::CArduinoSensorWithActivator(const std::string& ip, int port, const std::string& usr, const std::string& psw,
	                                                     int id, int request_threshold, const std::string& name) : CArduinoSensor(ip, port, usr, psw, id, request_threshold, name), CRemoteExecuter(), is_return_state(false)
{
}

CArduinoSensorWithActivator::~CArduinoSensorWithActivator()
{
}

ERemoteDeviceResponse CArduinoSensorWithActivator::DeActivate_internal() const
{
	is_return_state = false;
	return CArduinoDevice::DeActivate_internal();
}

ERemoteDeviceResponse CArduinoSensorWithActivator::GetSensorState_internal(ESensorState& state) const
{
	ERemoteDeviceResponse ret = TRUE_RESPONSE;

	if ( is_return_state )
		ret = CArduinoSensor::GetSensorState_internal(state);
	else
		state = ACTIVATED;

	return ret;
}

static const char ARDUINO_SENSOR_START_RETURN_EXECUTER_PARAMETER[] = "START";
static const char ARDUINO_SENSOR_STOP_RETURN_EXECUTER_PARAMETER[] = "STOP";

ERemoteDeviceResponse CArduinoSensorWithActivator::DoAction_internal(const void* param /*= NULL*/) const
{
	ERemoteDeviceResponse ret = FALSE_RESPONSE;

	if ( param )
	{
		ret = IsActivated();

		if ( ret == TRUE_RESPONSE )
		{
			if (_stricmp((const char*)param, ARDUINO_SENSOR_START_RETURN_EXECUTER_PARAMETER) == 0)
				is_return_state = true;
			else
			{
				if (_stricmp((const char*)param, ARDUINO_SENSOR_STOP_RETURN_EXECUTER_PARAMETER) == 0)
					is_return_state = false;
			}
		}
	}

	return ret;
}

