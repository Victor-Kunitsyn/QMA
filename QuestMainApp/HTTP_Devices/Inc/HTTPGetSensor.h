#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"
#include "CURLDownloaderLight.h"

class CHTTPGetSensor : public CRemoteDevice, public CRemoteSensor
{
public:
	CHTTPGetSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		           int id, int request_threshold, const std::string& name);

	virtual ~CHTTPGetSensor();

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
    virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const;
    virtual ERemoteDeviceResponse SetSensorState_internal(ESensorState state);

protected:
    std::string get_sensor_state_url;

    mutable CURLDownloaderLight downloader;
};

