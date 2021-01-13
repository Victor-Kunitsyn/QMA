#include "stdafx.h"

#include "QuestContextExecuter.h"

#include "QuestServer.h"

CQuestContextExecuter::CQuestContextExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
                                             int id, int request_threshold, const std::string& name) : 
                                             CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), main_server(NULL)
{
}

CQuestContextExecuter::~CQuestContextExecuter()
{
}

ERemoteDeviceResponse CQuestContextExecuter::Check_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestContextExecuter::Activate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestContextExecuter::DeActivate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestContextExecuter::IsActivated_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

enum EContextOpEnum
{
    LESS = 0,
    LARGER = 1,
    UNDEFINED = 2
};

// This is build-in executer for quest context operations

// Its execution parameters can be as followings:

// Key=Value - saves Key-Value strings pair to context. Both strings should not contain '='  

// Key;Value;Device ordinal number;Parameter - checks if context contains provided Key-Value
// and if it is so, calls DoAction with Parameter for device with ordinal number provided.
// All strings should not contain ';' and '='  

// Key;Value;Device ordinal number;Parameter;< - checks if context contains provided Key 
// and Value(Key) < Value as positive integers
// If it is so, calls DoAction with Parameter for device with ordinal number provided.
// All strings should not contain ';' and '='  

// Key;Value;Device ordinal number;Parameter;> - checks if context contains provided Key 
// and Value(Key) > Value as positive integers
// If it is so, calls DoAction with Parameter for device with ordinal number provided.
// All strings should not contain ';' and '='  

ERemoteDeviceResponse CQuestContextExecuter::DoAction_internal(const void* param/* = NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( (main_server != NULL) && (param != NULL) && (main_server->GetDeviceManager() != NULL) )
    {
        const char* param_str = (const char*)param;

        char* delim = (char*)(strchr(param_str, '='));
        
        if ( delim != NULL )
        {
            const char* value_str = delim + 1;
            *delim = 0x0;

            main_server->GetContext().AddKeyValue(param_str, value_str);

            ret = TRUE_RESPONSE;
        }
        else
        {
            char* delim1 = (char*)(strchr(param_str, ';'));
            
            if ( delim1 != NULL )
            {
                char* delim2 = (char*)(strchr(delim1 + 1, ';'));

                if ( delim2 != NULL )
                {
                    char* delim3 = (char*)(strchr(delim2 + 1, ';'));

                    if ( delim3 != NULL )
                    {
                        const char* key = param_str;
                        const char* value = delim1 + 1;
                        const char* exec_ordinal_str = delim2 + 1;
                        const char* exec_parameter = delim3 + 1;

                        *delim1 = 0x0;
                        *delim2 = 0x0;
                        *delim3 = 0x0;

                        char* delim4 = (char*)(strchr(exec_parameter, ';'));

                        if ( delim4 != NULL )
                        {
                            *delim4 = 0x0;

                            const char* operation = delim4 + 1;

                            EContextOpEnum operation_value = UNDEFINED;

                            if ( strcmp(operation, "<") == 0 )
                                operation_value = LESS;
                            else
                            {
                                if ( strcmp(operation, ">") == 0 )
                                    operation_value = LARGER;
                            }

                            if ( operation_value != UNDEFINED )
                            {
                                unsigned device_ordinal = atoi(exec_ordinal_str);

                                if ( device_ordinal < main_server->GetDeviceManager()->GetDevices().size() )
                                {
                                    CRemoteDevice* device = main_server->GetDeviceManager()->GetDevices()[device_ordinal];

                                    if ( device != NULL )
                                    {
                                        const std::string& context_value = main_server->GetContext().GetValue(key);

                                        unsigned context_val_int = atoi(context_value.c_str());
                                        unsigned val_int = atoi(value);

                                        if ( (context_value != "") && ( ((operation_value == LESS) && (context_val_int < val_int)) || 
                                                                        ((operation_value == LARGER) && (context_val_int > val_int)) ) )
                                        {
                                            ret = device->DoAction(main_server->GetDeviceManager(), exec_parameter);
                                        }
                                        else
                                            ret = TRUE_RESPONSE;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if ( main_server->GetContext().GetValue(key) == value )
                            {
                                unsigned device_ordinal = atoi(exec_ordinal_str);

                                if ( device_ordinal < main_server->GetDeviceManager()->GetDevices().size() )
                                {
                                    CRemoteDevice* device = main_server->GetDeviceManager()->GetDevices()[device_ordinal];

                                    if ( device != NULL )
                                    {
                                        ret = device->DoAction(main_server->GetDeviceManager(), exec_parameter);
                                    }
                                }
                            }
                            else
                                ret = TRUE_RESPONSE;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

