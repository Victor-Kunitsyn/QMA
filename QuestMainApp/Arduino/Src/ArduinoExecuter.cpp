#include "stdafx.h"
#include "ArduinoExecuter.h"

CArduinoExecuter::CArduinoExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
								   int id, int request_threshold, const std::string& name) : CArduinoDevice(ip, port, usr, psw, id, request_threshold, name), CRemoteExecuter()
{
}

CArduinoExecuter::~CArduinoExecuter()
{
}

//http parameters:

//do action: request ?I=id&T=E&P="parameter string" response "1" (true) or "0" (false)

ERemoteDeviceResponse CArduinoExecuter::DoAction_internal(const void* param /*= NULL*/) const
{
	CheckRequestTimeTreshold();

	device_cs.Enter();
	
	std::string request = "T=E&P=";
	if (param != NULL)
		request += (const char*)param;
	
	ERemoteDeviceResponse ret = ( SendRequestToServer(request.c_str()) ? TRUE_RESPONSE :  NO_RESPONSE );
	ret = ret && ( (o_result_xml == "1") ? TRUE_RESPONSE : (o_result_xml != "" ? FALSE_RESPONSE : NO_RESPONSE) );
    
	device_cs.Leave();

	return ret;
}
