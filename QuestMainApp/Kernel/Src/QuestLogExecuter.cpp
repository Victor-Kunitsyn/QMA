#include "stdafx.h"

#include "QuestLogExecuter.h"

#include "QuestServer.h"

CQuestLoggingExecuter::CQuestLoggingExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
                                             int id, int request_threshold, const std::string& name) : 
                                             CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), ptr_logger(NULL)
{
}

CQuestLoggingExecuter::~CQuestLoggingExecuter()
{
}

ERemoteDeviceResponse CQuestLoggingExecuter::Check_internal() const
{
    return ( (ptr_logger != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestLoggingExecuter::Activate_internal() const
{
    return ( (ptr_logger != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestLoggingExecuter::DeActivate_internal() const
{
    return ( (ptr_logger != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestLoggingExecuter::IsActivated_internal() const
{
    return ( (ptr_logger != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestLoggingExecuter::DoAction_internal(const void* param/* = NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( (ptr_logger != NULL) && (param != NULL) )
    {
        const char* msg = (const char*)param;
        ptr_logger->LogMessage(msg);

        ret = TRUE_RESPONSE;
    }

    return ret;
}

