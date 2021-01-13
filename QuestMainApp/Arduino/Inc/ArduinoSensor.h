#pragma once

#include "ArduinoDevice.h"
#include "QuestRemoteSensor.h"
#include "QuestRemoteExecuter.h"

class CArduinoSensor: public CArduinoDevice, public CRemoteSensor
{
public:
    CArduinoSensor(const std::string& ip, int port, const std::string& usr, const std::string& psw, int id, int request_threshold, const std::string& name);
    virtual ~CArduinoSensor();

protected:
    virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const;
    virtual ERemoteDeviceResponse SetSensorState_internal(ESensorState state);
};

class CArduinoSensorWithActivator : public CArduinoSensor, public CRemoteExecuter
{
public:
	CArduinoSensorWithActivator(const std::string& ip, int port, const std::string& usr, const std::string& psw, int id, int request_threshold, const std::string& name);
	virtual ~CArduinoSensorWithActivator();

protected:
	virtual ERemoteDeviceResponse DeActivate_internal() const;

protected:
	virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const;

protected:
	virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
	mutable bool is_return_state;
};