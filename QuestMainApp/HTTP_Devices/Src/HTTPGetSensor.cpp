#include "stdafx.h"

#include "HTTPGetSensor.h"

CHTTPGetSensor::CHTTPGetSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw,
	                           int id, int request_threshold, const std::string& name) : 
	                           CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), get_sensor_state_url(ip)
{
	downloader.UseProxy("", "");
}

CHTTPGetSensor::~CHTTPGetSensor()
{
}

ERemoteDeviceResponse CHTTPGetSensor::Check_internal() const
{
	return TRUE_RESPONSE;
}

ERemoteDeviceResponse CHTTPGetSensor::IsActivated_internal() const
{
	return TRUE_RESPONSE;
}

ERemoteDeviceResponse CHTTPGetSensor::Activate_internal() const
{
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CHTTPGetSensor::DeActivate_internal() const
{
    return TRUE_RESPONSE;
}

static void WriteXMLtoString(void* p_string, const char* buffer, int len)
{
	std::string& str = *((std::string*)p_string);
	str.append(buffer, len);
}

ERemoteDeviceResponse CHTTPGetSensor::GetSensorState_internal(ESensorState& state) const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;

    ERemoteDeviceResponse is_activated = IsActivated();

    if ( is_activated == TRUE_RESPONSE )
    {
        downloader.SetURL(get_sensor_state_url.c_str());

        std::string o_result_xml;

        if ( downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms) == CURLDownloaderLight::UR_NO_ERROR )
        {
            if ( o_result_xml != "" )
            {
                int state_dig = atoi(o_result_xml.c_str());
                state = ESensorState((ESensorStateEnum)state_dig);
            }
            else
                ret = NO_RESPONSE;
        }
        else
            ret = NO_RESPONSE;
    }
    else
    {
        if ( is_activated == FALSE_RESPONSE )
            state = DEACTIVATED;
        else
            ret = NO_RESPONSE;
    }

    return ret;
}

ERemoteDeviceResponse CHTTPGetSensor::SetSensorState_internal(ESensorState state)
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
    
    if ( state != DEACTIVATED )
        ret = Activate();

    return ret;
}
