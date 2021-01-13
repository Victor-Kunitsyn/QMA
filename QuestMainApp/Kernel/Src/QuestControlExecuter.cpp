#include "stdafx.h"

#include "QuestControlExecuter.h"

#include "QuestServer.h"

CQuestControlExecuter::CQuestControlExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
                                             int id, int request_threshold, const std::string& name) : 
                                             CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), main_server(NULL)
{
}

CQuestControlExecuter::~CQuestControlExecuter()
{
}

ERemoteDeviceResponse CQuestControlExecuter::Check_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestControlExecuter::Activate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestControlExecuter::DeActivate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestControlExecuter::IsActivated_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

const static char QUEST_ACTIVATE_PARAMETER[] =             "QUEST_ACTIVATE";
const static char QUEST_ACTIVATE_NEXT_STAGE_PARAMETER[] =  "QUEST_ACTIVATE_NEXT_STAGE";
const static char QUEST_DEACTIVATE_PARAMETER[] =           "QUEST_DEACTIVATE";
const static char QUEST_PAUSE_PARAMETER[] =                "QUEST_PAUSE";
const static char QUEST_RESUME_PARAMETER[] =               "QUEST_RESUME";

// This is build-in executer for quest control operations

// Its execution parameters can be as followings:

// QUEST_ACTIVATE, QUEST_ACTIVATE_NEXT_STAGE, QUEST_DEACTIVATE, QUEST_PAUSE, QUEST_RESUME - performs corresponding quest control action

// QUEST_PAUSE;Sleep_ms;Resume_ms - sets Quest on pause, then sleep for Sleep_ms milliseconds.
// Then thread execution is continued and after Resume_ms milliseconds (after Quest pause, disregarding Sleep_ms!!!) Quest resumed

ERemoteDeviceResponse CQuestControlExecuter::DoAction_internal(const void* param/* = NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( (main_server != NULL) && (param != NULL) )
    {
        const char* param_str = (const char*)param;

        char* delim1 = (char*)(strchr(param_str, ';'));

        if ( delim1 != NULL )
        {
            char* delim2 = (char*)(strchr(delim1 + 1, ';'));

            if ( delim2 != NULL )
            {
                const char* action_str = param_str;
                const char* sleep_ms_str = delim1 + 1;
                const char* resume_ms_str = delim2 + 1;

                *delim1 = 0x0;
                *delim2 = 0x0;

                if (_stricmp(action_str, QUEST_PAUSE_PARAMETER) == 0 )
                {
                    DWORD sleep_ms = atol(sleep_ms_str);
                    DWORD resume_ms = atol(resume_ms_str);

                    if ( main_server->PauseSystem() == OK )
                    {
                        ::Sleep(sleep_ms);

                        main_server->SetResumeInTime(resume_ms);

                        ret = TRUE_RESPONSE;
                    }
                }
            }
        }
        else
        {
            if (_stricmp(param_str, QUEST_ACTIVATE_PARAMETER) == 0 ) ret = ( (main_server->ActivateSystem() == OK) ? TRUE_RESPONSE : FALSE_RESPONSE);
            else if (_stricmp(param_str, QUEST_ACTIVATE_NEXT_STAGE_PARAMETER) == 0 ) ret = ( (main_server->ActivateNextStage() == OK) ? TRUE_RESPONSE : FALSE_RESPONSE );
            else if (_stricmp(param_str, QUEST_DEACTIVATE_PARAMETER) == 0)
            {
                main_server->GetLogger()->LogMessage("---------GAME END : EXTERNAL BREAK---------");
                main_server->GetLogger()->LogNumberWithPattern(main_server->GetSystemActivationTime(), "GAME ID: %u");
                main_server->GetLogger()->LogStatistics(EXTERNAL_BREAK_GT, main_server->GetPlayersNum(), main_server->GetElapsedTimeMin());

                ret = ((main_server->DeActivateSystem() == OK) ? TRUE_RESPONSE : FALSE_RESPONSE);
            }
            else if (_stricmp(param_str, QUEST_PAUSE_PARAMETER) == 0 ) ret = ( (main_server->PauseSystem() == OK) ? TRUE_RESPONSE : FALSE_RESPONSE);
            else if (_stricmp(param_str, QUEST_RESUME_PARAMETER) == 0 ) ret = ( (main_server->ResumeSystem() == OK) ? TRUE_RESPONSE : FALSE_RESPONSE);
        }
    }

    return ret;
}


