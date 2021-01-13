#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

//Special build-in Kernel executer to perform Quest control operations: Activate/Deactivate, Pause/Resume 
//Its type should be marked by CONTROL_BUILD_IN_EXECUTER_TYPE in Quest *.ini file

#define CONTROL_BUILD_IN_EXECUTER_TYPE "CONTROL_BUILD_IN_EXECUTER"

class CMainServer;

class CQuestControlExecuter : public CRemoteDevice, public CRemoteExecuter
{
public:
	CQuestControlExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		                  int id, int request_threshold, const std::string& name);

	virtual ~CQuestControlExecuter();

    void SetOwner(CMainServer* main_server_ptr) { main_server = main_server_ptr; }

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
	virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    CMainServer* main_server;
};

