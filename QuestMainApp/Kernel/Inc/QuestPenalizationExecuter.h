#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

class CMainServer;

//Special build-in Kernel executer to perform time different penalizations
//Its type should be marked by PENALIZATION_BUILD_IN_EXECUTER_TYPE in Quest *.ini file

#define PENALIZATION_BUILD_IN_EXECUTER_TYPE "PENALIZATION_BUILD_IN_EXECUTER"

class CQuestPenalizationExecuter : public CRemoteDevice, public CRemoteExecuter
{
public:
	CQuestPenalizationExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		                       int id, int request_threshold, const std::string& name);

	virtual ~CQuestPenalizationExecuter();

    void SetOwner(const CMainServer* main_server_ptr) { main_server = main_server_ptr; }

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
	virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    const CMainServer* main_server;
};

