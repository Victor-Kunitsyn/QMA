#include "stdafx.h"

#include "HTTPGetExecuter.h"

CHTTPGetExecuter::CHTTPGetExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
	                               int id, int request_threshold, const std::string& name) : 
	                               CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), 
                                   is_activated(false), activation_url(ip), executer_parameter_prefix(usr), deactivation_url(psw)
{
	downloader.UseProxy("", "");
}

CHTTPGetExecuter::~CHTTPGetExecuter()
{
}

ERemoteDeviceResponse CHTTPGetExecuter::Check_internal() const
{
	return TRUE_RESPONSE;
}

ERemoteDeviceResponse CHTTPGetExecuter::IsActivated_internal() const
{
	return ( is_activated ? TRUE_RESPONSE : FALSE_RESPONSE );
}

static void WriteXMLtoString(void* p_string, const char* buffer, int len)
{
	std::string& str = *((std::string*)p_string);
	str.append(buffer, len);
}

ERemoteDeviceResponse CHTTPGetExecuter::Activate_internal() const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;

    if ( !is_activated )
    {
        if ( activation_url != "" )
        {
            downloader.SetURL(activation_url.c_str());

            std::string o_result_xml;

            if ( downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms) != CURLDownloaderLight::UR_NO_ERROR )
                ret = NO_RESPONSE;
        }

        if ( ret == TRUE_RESPONSE )
            is_activated = true;
    }

    return ret;
}

ERemoteDeviceResponse CHTTPGetExecuter::DeActivate_internal() const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
	
    if ( deactivation_url != "" )
    {
        downloader.SetURL(deactivation_url.c_str());
        
        std::string o_result_xml;

        if ( downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms) != CURLDownloaderLight::UR_NO_ERROR )
            ret = NO_RESPONSE;
    }

    is_activated = false;

    return ret;
}

ERemoteDeviceResponse CHTTPGetExecuter::DoAction_internal(const void* param /*= NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( is_activated )
    {
        std::string get_url = executer_parameter_prefix;

        if (param != NULL)
            get_url += (const char*)param;

        downloader.SetURL(get_url.c_str());
        std::string o_result_xml;

        ret = ( (downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms) == CURLDownloaderLight::UR_NO_ERROR) ? TRUE_RESPONSE : NO_RESPONSE);
    }

    return ret;
}

