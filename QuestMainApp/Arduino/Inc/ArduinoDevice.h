#pragma once

#include "QuestRemoteDevice.h"
#include "CURLDownloaderLight.h"

class CArduinoDevice : public CRemoteDevice
{
public:
    CArduinoDevice(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
		           int id, int request_threshold, const std::string& name);
    
	virtual ~CArduinoDevice();

protected:
	static void WriteXMLtoString(void* p_string, const char* buffer, int len);
	bool SendRequestToServer(const std::string& request_string) const;

protected:
    virtual ERemoteDeviceResponse Check_internal() const;
    virtual ERemoteDeviceResponse Activate_internal() const;
    virtual ERemoteDeviceResponse DeActivate_internal() const;
    virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
	std::string device_http_addr;
	mutable CURLDownloaderLight downloader;
	mutable std::string o_result_xml;
};