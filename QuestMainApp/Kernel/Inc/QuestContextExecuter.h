#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

//Special build-in Kernel executer to perform Quest context operations - saving key-values and checking against them
//Its type should be marked by CONTEXT_BUILD_IN_EXECUTER_TYPE in Quest *.ini file

#define CONTEXT_BUILD_IN_EXECUTER_TYPE "CONTEXT_BUILD_IN_EXECUTER"

class CMainServer;

class CQuestContextExecuter : public CRemoteDevice, public CRemoteExecuter
{
public:
	CQuestContextExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		                  int id, int request_threshold, const std::string& name);

	virtual ~CQuestContextExecuter();

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

