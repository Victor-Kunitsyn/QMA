#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

//Special build-in Kernel executer to perform logging operations (for example to display user some useful info)
//Its type should be marked by LOGGING_BUILD_IN_EXECUTER_TYPE in Quest *.ini file

#define LOGGING_BUILD_IN_EXECUTER_TYPE "LOGGING_BUILD_IN_EXECUTER"

class CLogger;

class CQuestLoggingExecuter : public CRemoteDevice, public CRemoteExecuter
{
public:
	CQuestLoggingExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		                  int id, int request_threshold, const std::string& name);

	virtual ~CQuestLoggingExecuter();

    void SetLogger(const CLogger* ptr_logger_p) { ptr_logger = ptr_logger_p; }

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
	virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    const CLogger* ptr_logger;
};


