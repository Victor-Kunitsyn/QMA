#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

class CQuestMainAppOutputWindowDlg;

class COutputWindowExecuter: public CRemoteDevice, public CRemoteExecuter
{
public:
    COutputWindowExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
		                  int id, int request_threshold, const std::string& name);
    
	virtual ~COutputWindowExecuter();

protected:
    virtual ERemoteDeviceResponse Check_internal() const;
    virtual ERemoteDeviceResponse Activate_internal() const;
    virtual ERemoteDeviceResponse DeActivate_internal() const;
    virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
    virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    mutable bool is_activated;
    
    mutable CQuestMainAppOutputWindowDlg* output_dlg;
};
