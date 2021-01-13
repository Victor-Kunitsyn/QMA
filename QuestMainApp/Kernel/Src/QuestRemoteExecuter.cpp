#include "stdafx.h"

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"

#include "QuestServer.h"
#include "QuestRemoteDevicesManager.h"

static const char REMOTE_EXECUTER_NUM_PLAYERS_PARAMETER[] = "NUM_PLAYERS";
static const char REMOTE_EXECUTER_REST_TIME_PARAMETER[] = "REST_TIME";
static const char REMOTE_EXECUTER_LOCAL_CURRENT_HOUR_PARAMETER[] = "LOCAL_CURRENT_HOUR";
static const char REMOTE_EXECUTER_LOCAL_CURRENT_MINUTE_PARAMETER[] = "LOCAL_CURRENT_MINUTE";
static const char REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER[] = "SENSOR_VALUE_";
static const char REMOTE_EXECUTER_CONTEXT_VALUE_PARAMETER[] = "CONTEXT_VALUE_";

std::string CRemoteExecuter::TransformExecuterParameter(const void* param, const CRemoteDevicesManager* ptr_device_mgr)
{
    std::string param_str = "";

    if ( (param != NULL) && (ptr_device_mgr != NULL) )
    {
        const CMainServer* ptr_server = ptr_device_mgr->GetServer();

        param_str = (const char*)param;
        std::string::size_type pos_to_replace = std::string::npos;

        char tmp_buffer[12];

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_NUM_PLAYERS_PARAMETER)) != std::string::npos )
        {
            sprintf(tmp_buffer, "%u", ptr_server->GetPlayersNum());

            param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_NUM_PLAYERS_PARAMETER), tmp_buffer);
        }

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_REST_TIME_PARAMETER)) != std::string::npos )
        {
            int real_elapsed_time_ms;
            DWORD remain_time_ms = ptr_server->GetRemainTimeMs(real_elapsed_time_ms);
            sprintf(tmp_buffer, "%u", remain_time_ms / 1000);

            param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_REST_TIME_PARAMETER), tmp_buffer);
        }

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_LOCAL_CURRENT_HOUR_PARAMETER)) != std::string::npos )
        {
            SYSTEMTIME cur_local_time;
            ::GetLocalTime(&cur_local_time);
            sprintf(tmp_buffer, "%u", cur_local_time.wHour);

            param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_LOCAL_CURRENT_HOUR_PARAMETER), tmp_buffer);
        }

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_LOCAL_CURRENT_MINUTE_PARAMETER)) != std::string::npos )
        {
            SYSTEMTIME cur_local_time;
            ::GetLocalTime(&cur_local_time);
            sprintf(tmp_buffer, "%u", cur_local_time.wMinute);

            param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_LOCAL_CURRENT_MINUTE_PARAMETER), tmp_buffer);
        }

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER)) != std::string::npos )
        {
            std::string::size_type pos_of_sensor_num = pos_to_replace + strlen(REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER);

            const char* sensor_num_str = param_str.c_str() + pos_of_sensor_num;
            const char* sensor_num_str_end = sensor_num_str;

            while ( isdigit(*sensor_num_str_end) ) { sensor_num_str_end++; }

            if ( sensor_num_str_end > sensor_num_str ) //digits detected
            {
                std::string sensor_ord_substr = param_str.substr(pos_of_sensor_num, sensor_num_str_end - sensor_num_str);
                unsigned sensor_ord = atoi(sensor_ord_substr.c_str());

                if ( sensor_ord < ptr_device_mgr->GetDevices().size() )
                {
                    unsigned sensor_value = ptr_device_mgr->GetSensorStates()[sensor_ord].GetValue();
                    sprintf(tmp_buffer, "%u", sensor_value);

                    param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER) + (sensor_num_str_end - sensor_num_str), tmp_buffer);
                }
                else
                    param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER), "INVALID SENSOR NUMBER");
            }
            else
            {
                param_str.replace(pos_to_replace, strlen(REMOTE_EXECUTER_SENSOR_VALUE_PARAMETER), "UNDEFINED SENSOR NUMBER");
            }
        }

        while ( (pos_to_replace = param_str.find(REMOTE_EXECUTER_CONTEXT_VALUE_PARAMETER)) != std::string::npos )
        {
            std::string::size_type pos_of_key = pos_to_replace + strlen(REMOTE_EXECUTER_CONTEXT_VALUE_PARAMETER);

            const char* key_str = param_str.c_str() + pos_of_key;
            const std::string& value = ptr_server->GetContext().GetValue(key_str);

            param_str.replace(pos_to_replace, param_str.length() - pos_to_replace, value.c_str());
        }
    }

    return param_str;
}

static const char REMOTE_EXECUTER_NO_EXECUTION_PARAMETER[] = "NO_EXECUTION";

ERemoteDeviceResponse CRemoteExecuter::DoAction(const void* param /*= NULL*/, const CRemoteDevicesManager* ptr_device_mgr /*= NULL*/) const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;

    if ( !IsDeviceDisregardered() )
    {
        if ( (param == NULL) || (strcmp((const char*)param, REMOTE_EXECUTER_NO_EXECUTION_PARAMETER) != 0) )
        {
            std::string param_str = "";

            if ( (param != NULL) && (ptr_device_mgr != NULL) )
            {
                param_str = TransformExecuterParameter(param, ptr_device_mgr);
                param = param_str.c_str();
            }

            ret = NO_RESPONSE;

            for ( unsigned cnt = 0; (cnt < CRemoteDevice::GetMaxNumOfRequestsIfNoResponse()) && (ret == NO_RESPONSE); cnt++ )
                ret = DoAction_internal(param);
        }
    }

    return ret;
}

