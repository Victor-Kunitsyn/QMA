#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"
#include "CURLDownloaderLight.h"

class CHTTPGetExecuter : public CRemoteDevice, public CRemoteExecuter
{
public:
	CHTTPGetExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		             int id, int request_threshold, const std::string& name);

	virtual ~CHTTPGetExecuter();

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
	virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    mutable bool is_activated;

    std::string activation_url;
    std::string deactivation_url;
    std::string executer_parameter_prefix;

    mutable CURLDownloaderLight downloader;
};