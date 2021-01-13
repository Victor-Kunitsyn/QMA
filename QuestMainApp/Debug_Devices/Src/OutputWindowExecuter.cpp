#include "stdafx.h"

#include "OutputWindowExecuter.h"

#include "QuestMainAppOutputWindowDlg.h"

COutputWindowExecuter::COutputWindowExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
							                 int id, int request_threshold, const std::string& name) : CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), 
                                                                                                       CRemoteExecuter(),
                                                                                                       is_activated(false)
{
    output_dlg = new CQuestMainAppOutputWindowDlg();
    output_dlg->Create();
}

COutputWindowExecuter::~COutputWindowExecuter()
{
    delete output_dlg;
}

ERemoteDeviceResponse COutputWindowExecuter::Check_internal() const
{
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse COutputWindowExecuter::Activate_internal() const
{
    if ( !is_activated )
    {
		is_activated = true;
        output_dlg->SetText("");
    }
    
	return TRUE_RESPONSE;
}

ERemoteDeviceResponse COutputWindowExecuter::DeActivate_internal() const
{
    is_activated = false;
    output_dlg->SetText("");

    return TRUE_RESPONSE;
}

ERemoteDeviceResponse COutputWindowExecuter::IsActivated_internal() const
{
    return ( is_activated ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse COutputWindowExecuter::DoAction_internal(const void* param /*= NULL*/) const
{
    if ( is_activated )
	{
		const char* str = (const char*)param;

        if ( str )
            output_dlg->SetText(str);
        
        return TRUE_RESPONSE;
	}
	else
		return FALSE_RESPONSE;
}
