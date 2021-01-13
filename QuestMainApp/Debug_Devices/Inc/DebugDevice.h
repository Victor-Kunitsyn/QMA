#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"
#include "QuestRemoteExecuter.h"

class CDebugSensor: public CRemoteDevice, public CRemoteSensor
{
public:
    CDebugSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
			     int id, int request_threshold, const std::string& name);
    
	virtual ~CDebugSensor();

protected:
    virtual ERemoteDeviceResponse Check_internal() const;
    virtual ERemoteDeviceResponse Activate_internal() const;
    virtual ERemoteDeviceResponse DeActivate_internal() const;
    virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
    virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const;
    virtual ERemoteDeviceResponse SetSensorState_internal(ESensorState state);

protected:
    mutable bool is_activated;
    mutable ESensorState sensor_state;
};

class CDebugExecuter: public CRemoteDevice, public CRemoteExecuter
{
public:
    CDebugExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
		           int id, int request_threshold, const std::string& name);
    
	virtual ~CDebugExecuter();

protected:
    virtual ERemoteDeviceResponse Check_internal() const;
    virtual ERemoteDeviceResponse Activate_internal() const;
    virtual ERemoteDeviceResponse DeActivate_internal() const;
    virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
    virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    mutable bool is_activated;
};