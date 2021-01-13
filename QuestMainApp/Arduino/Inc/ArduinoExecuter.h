#pragma once

#include "ArduinoDevice.h"
#include "QuestRemoteExecuter.h"

class CArduinoExecuter: public CArduinoDevice, public CRemoteExecuter
{
public:
    CArduinoExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, int id, int request_threshold, const std::string& name);
    virtual ~CArduinoExecuter();

protected:
    virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;
};