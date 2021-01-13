#include "stdafx.h"
#include "ArduinoDevice.h"

#include "QuestCommon.h"

CArduinoDevice::CArduinoDevice(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
							   int id, int request_threshold, const std::string& name) : CRemoteDevice(ip, port, usr, psw, id, request_threshold, name)
{
	downloader.UseProxy("", "");
	
	device_http_addr = "http://";
	device_http_addr += device_ip;
	device_http_addr += ":";
	char temp_buffer[10];
	sprintf(temp_buffer, "%d", device_port);
	device_http_addr += temp_buffer;
	device_http_addr += "?I=";
	sprintf(temp_buffer, "%d", device_id);
	device_http_addr += temp_buffer;
	device_http_addr += "&";
}

CArduinoDevice::~CArduinoDevice()
{
}

void CArduinoDevice::WriteXMLtoString(void* p_string, const char* buffer, int len)
{
	std::string& str = *((std::string*)p_string);
	str.append(buffer, len);
}

static CScopedCriticalSection cs;

bool CArduinoDevice::SendRequestToServer(const std::string& request_string) const
{
	o_result_xml = "";
	
	cs.Enter();

	bool ret = downloader.SetURL((device_http_addr + request_string).c_str());
	ret = ret && (downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms) == CURLDownloaderLight::UR_NO_ERROR);

	cs.Leave();
	
	return ret;
}

//http parameters:

//check: request ?I=id&T=C response "1" (true) or "0" (false)
//activate: request ?I=id&T=A response "1" (true) or "0" (false)
//deactivate: request ?I=id&T=D response "1" (true) or "0" (false)
//is activated: request ?I=id&T=I response "1" (activated) or "0" (not activated)

ERemoteDeviceResponse CArduinoDevice::Check_internal() const
{
	device_cs.Enter();
	ERemoteDeviceResponse ret = ( SendRequestToServer("T=C") ? TRUE_RESPONSE :  NO_RESPONSE );
    ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
	device_cs.Leave();
	return ret;
}

ERemoteDeviceResponse CArduinoDevice::Activate_internal() const
{
	device_cs.Enter();
	ERemoteDeviceResponse ret = ( SendRequestToServer("T=A") ? TRUE_RESPONSE :  NO_RESPONSE );
	ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
	device_cs.Leave();
	return ret;
}

ERemoteDeviceResponse CArduinoDevice::DeActivate_internal() const
{
	device_cs.Enter();
	ERemoteDeviceResponse ret = ( SendRequestToServer("T=D") ? TRUE_RESPONSE :  NO_RESPONSE );
	ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
	device_cs.Leave();
	return ret;
}

ERemoteDeviceResponse CArduinoDevice::IsActivated_internal() const
{
	device_cs.Enter();
	ERemoteDeviceResponse ret = ( SendRequestToServer("T=I") ? TRUE_RESPONSE :  NO_RESPONSE );
	ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
	device_cs.Leave();
	return ret;
}

