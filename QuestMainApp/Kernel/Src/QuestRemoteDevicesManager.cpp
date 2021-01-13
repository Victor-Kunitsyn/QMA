#include "stdafx.h"
#include <strstream>

#include "QuestRemoteDevicesManager.h"
#include "QuestServer.h"
#include "QuestRemoteExecuter.h"

#include "QuestPenalizationExecuter.h"
#include "QuestTimeDevice.h"
#include "QuestControlExecuter.h"
#include "QuestContextExecuter.h"
#include "QuestLogExecuter.h"

std::string GetResponseString(ERemoteDeviceResponse device_response)
{
    std::string ret = "";

    switch (device_response)
    {
    case FALSE_RESPONSE:
        ret = "Ответ отрицательный";
        break;
    case NO_RESPONSE:
        ret = "Нет ответа";
        break;
    case TRUE_RESPONSE:
        ret = "Ответ положительный";
        break;
    }

    return ret;
}

CRemoteDevicesManager::CRemoteDevicesManager(const CMainServer* main_server_ptr) : main_server(main_server_ptr),
                                                                                   max_number_of_sensor_alarms(NULL), 
                                                                                   sensor_alarms_count(NULL), 
                                                                                   sensors_max_times_in_alarm_mode(NULL),
                                                                                   sensors_start_alarm_times(NULL),
                                                                                   connection_processings_count(NULL),
                                                                                   sensor_states(NULL),
                                                                                   current_states(NULL),
                                                                                   should_restore_sensor_states(true),
                                                                                   int_get_sensor_state_attempts(0),
                                                                                   current_device_index_to_activate(0),
                                                                                   ptr_logger(main_server_ptr->GetLogger())
{
}

CRemoteDevicesManager::~CRemoteDevicesManager()
{
    Clear();
}

void CRemoteDevicesManager::Clear()
{
    for ( int i = 0; i < devices.size(); i++ )
        delete devices[i];

    devices.clear();
    sensor_connections.clear();
    
    delete[] max_number_of_sensor_alarms;
    max_number_of_sensor_alarms = NULL;

    delete[] sensor_alarms_count;
    sensor_alarms_count = NULL;

    delete[] sensors_max_times_in_alarm_mode;
    sensors_max_times_in_alarm_mode = NULL;

    delete[] sensors_start_alarm_times;
    sensors_start_alarm_times = NULL;
 
    delete[] connection_processings_count;
    connection_processings_count = NULL;
        
    delete[] sensor_states;
    sensor_states = NULL;

    delete[] current_states;
    current_states = NULL;

    should_restore_sensor_states = true;

    int_get_sensor_state_attempts = 0;

    devices_execute_actions.clear();
    devices_saved_execution_parameters.clear();

    current_device_index_to_activate = 0;
}

const static char SENSOR_EVENT_LISTENING_CONTINUE[] = "LISTENING_CONTINUE";
const static char SENSOR_EVENT_ALARM_ON[] = "ALARM_ON";
const static char SENSOR_EVENT_ALARM_OFF[] = "ALARM_OFF";
const static char SENSOR_EVENT_ALARM_CONTINUE[] = "ALARM_CONTINUE";
const static char SENSOR_EVENT_DISARM_ON[] = "DISARM_ON";
const static char SENSOR_EVENT_DISARM_OFF[] = "DISARM_OFF";
const static char SENSOR_EVENT_DISARM_CONTINUE[] = "DISARM_CONTINUE";
const static char SENSOR_EVENT_SLEEP_ON[] = "SLEEP_ON";
const static char SENSOR_EVENT_SLEEP_OFF[] = "SLEEP_OFF";
const static char SENSOR_EVENT_SLEEP_CONTINUE[] = "SLEEP_CONTINUE";

struct SEventStrToEvent
{
    SEventStrToEvent(const char* str, ESensorEvent s_e) : sensor_event_str(str), sensor_event(s_e) {}
    
    const char* sensor_event_str;
    ESensorEvent sensor_event;
};

const static SEventStrToEvent event_str_to_event[LAST_SENSOR_EVENT] = 
{
    SEventStrToEvent( SENSOR_EVENT_LISTENING_CONTINUE, LISTENING_CONTINUE ),
    SEventStrToEvent( SENSOR_EVENT_ALARM_ON, ALARM_ON ),
    SEventStrToEvent( SENSOR_EVENT_ALARM_OFF, ALARM_OFF ),
    SEventStrToEvent( SENSOR_EVENT_ALARM_CONTINUE, ALARM_CONTINUE ),
    SEventStrToEvent( SENSOR_EVENT_DISARM_ON, DISARM_ON ),
    SEventStrToEvent( SENSOR_EVENT_DISARM_OFF, DISARM_OFF ),
    SEventStrToEvent( SENSOR_EVENT_DISARM_CONTINUE, DISARM_CONTINUE ),
    SEventStrToEvent( SENSOR_EVENT_SLEEP_ON, SLEEP_ON ),
    SEventStrToEvent( SENSOR_EVENT_SLEEP_OFF, SLEEP_OFF ),
    SEventStrToEvent( SENSOR_EVENT_SLEEP_CONTINUE, SLEEP_CONTINUE )
};

static ESensorEvent GetSensorEvent(const char* sensor_event)
{
    ESensorEvent ret = LAST_SENSOR_EVENT;

    for ( int i = 0; i < LAST_SENSOR_EVENT; i++ )
    {
        if ( strcmp(sensor_event, event_str_to_event[i].sensor_event_str) == 0 ) 
        {
            ret = event_str_to_event[i].sensor_event;
            break;
        }
    }
    
    return ret;
}

const static char DEVICE_SECT[] = "DEVICES";
const static char DEVICE_NUM_KEY[] = "NUM";
const static char DEVICE_SECT_PATTERN[] = "DEVICE_%d";
const static char DEVICE_TYPE_KEY[] = "TYPE";
const static char DEVICE_IP_KEY[] = "IP";
const static char DEVICE_PORT_KEY[] = "PORT";
const static char DEVICE_USER_KEY[] = "USER";
const static char DEVICE_PASSWORD_KEY[] = "PASSWORD";
const static char DEVICE_ID_KEY[] = "ID";
const static char DEVICE_MIN_REQUEST_THRESHOLD_KEY[] = "MIN_TIME_MS_BETWEEN_REQUESTS";
const static char DEVICE_NAME_KEY[] = "NAME";
const static char DEVICE_MAX_ALARMS_KEY[] = "MAX_ALARM_NUM";
const static char DEVICE_MAX_TIME_IN_ALARM_STATE_KEY[] = "MAX_TIME_IN_ALARM_STATE_MS";
const static char DEVICE_SHOULD_BE_ALWAYS_ACTIVATED_KEY[] = "SHOULD_BE_ALWAYS_ACTIVATED";
const static char DEVICE_CONNECTION_TIMEOUT_KEY[] = "CONNECTION_TIMEOUT_MS";
const static char DEVICE_DATA_TIMEOUT_KEY[] = "DATA_TIMEOUT_MS";

const static char DEVICE_CONNECTIONS_SECT[] = "DEVICE_CONNECTIONS";
const static char DEVICE_CONNECTIONS_NUM_KEY[] = "NUM";
const static char DEVICE_CONNECTION_SECT_PATTERN[] = "CONNECTION_%d";
const static char DEVICE_CONNECTIONS_SENSORS_KEY[] = "SENSORS";
const static char DEVICE_CONNECTIONS_EXECUTER_KEY[] = "EXECUTER";
const static char DEVICE_CONNECTIONS_EXECUTER_PARAM_KEY_PATTERN[] = "EXECUTER_PARAM_%d";
const static char DEVICE_SHOULD_REDO_IF_DEACTIVATED_KEY[] = "SHOULD_REDO_IF_DEACTIVATED";
const static char DEVICE_SHOULD_LOG_EXECUTION_KEY[] = "SHOULD_LOG_EXECUTION";
const static char DEVICE_TRANSFORM_PARAMETER_ON_EXECUTION_TIME_KEY[] = "TRANSFORM_PARAMETER_ON_EXECUTION_TIME";

bool CRemoteDevicesManager::InitDevices(int num_devices, const char* ini_file_full_path, ptr_Device_Creator remote_device_factory)
{
    bool ret = true;

    char current_sect[30];
    char output_buffer[1024];

    for ( int i = 0; (i < num_devices) && ret; i++ )
    {
        sprintf(current_sect, DEVICE_SECT_PATTERN, i);

        ::GetPrivateProfileString(current_sect, DEVICE_TYPE_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);
        std::string device_type_str = output_buffer;

        ::GetPrivateProfileString(current_sect, DEVICE_IP_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);
        std::string device_ip = output_buffer;

        int device_port = ::GetPrivateProfileInt(current_sect, DEVICE_PORT_KEY, 0, ini_file_full_path);

        ::GetPrivateProfileString(current_sect, DEVICE_USER_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);
        std::string device_user = output_buffer;

        ::GetPrivateProfileString(current_sect, DEVICE_PASSWORD_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);
        std::string device_password = output_buffer;

        int device_id = ::GetPrivateProfileInt(current_sect, DEVICE_ID_KEY, -1, ini_file_full_path);

        if ( device_id < 0 )
        {
            ret = false;
            break;
        }

        int device_min_request_threshold_ms = ::GetPrivateProfileInt(current_sect, DEVICE_MIN_REQUEST_THRESHOLD_KEY, 0, ini_file_full_path);

        ::GetPrivateProfileString(current_sect, DEVICE_NAME_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);
        std::string device_name = output_buffer;

        CRemoteDevice* device = remote_device_factory(device_type_str.c_str(), device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);

        if ( device == NULL ) //try to use build-in Kernel devices
        {
            if ( strcmp(device_type_str.c_str(), PENALIZATION_BUILD_IN_EXECUTER_TYPE) == 0 )
            {
                CQuestPenalizationExecuter* penalization_executer = new CQuestPenalizationExecuter(device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);
                penalization_executer->SetOwner(main_server);
                device = penalization_executer;
            }
            else
            {
                if ( strcmp(device_type_str.c_str(), TIME_CONDITIONAL_BUILD_IN_DEVICE_TYPE) == 0 )
                {
                    CQuestTimeConditionalDevice* timer_device = new CQuestTimeConditionalDevice(device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);
                    timer_device->SetOwner(main_server);
                    device = timer_device;
                }
                else
                {
                    if ( strcmp(device_type_str.c_str(), CONTROL_BUILD_IN_EXECUTER_TYPE) == 0 )
                    {
                        CQuestControlExecuter* control_device = new CQuestControlExecuter(device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);
                        control_device->SetOwner((CMainServer*)main_server);
                        device = control_device;
                    }
                    else
                    {
                        if ( strcmp(device_type_str.c_str(), CONTEXT_BUILD_IN_EXECUTER_TYPE) == 0 )
                        {
                            CQuestContextExecuter* context_device = new CQuestContextExecuter(device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);
                            context_device->SetOwner((CMainServer*)main_server);
                            device = context_device;
                        }
                        else
                        {
                            if ( strcmp(device_type_str.c_str(), LOGGING_BUILD_IN_EXECUTER_TYPE) == 0 )
                            {
                                CQuestLoggingExecuter* logging_device = new CQuestLoggingExecuter(device_ip, device_port, device_user, device_password, device_id, device_min_request_threshold_ms, device_name);
                                logging_device->SetLogger(main_server->GetLogger());
                                device = logging_device;
                            }
                        }
                    }
                }
            }
        }

        if ( device )
        {
            bool should_be_always_activated = (::GetPrivateProfileInt(current_sect, DEVICE_SHOULD_BE_ALWAYS_ACTIVATED_KEY, 0, ini_file_full_path) != 0);
            device->should_be_always_activated = should_be_always_activated;

            unsigned long connection_timeout_ms = ::GetPrivateProfileInt(current_sect, DEVICE_CONNECTION_TIMEOUT_KEY, DEFAULT_CURL_TIMEOUTS_MS, ini_file_full_path);
            device->connection_timeout_ms = connection_timeout_ms;
            
            unsigned long data_timeout_ms = ::GetPrivateProfileInt(current_sect, DEVICE_DATA_TIMEOUT_KEY, DEFAULT_CURL_TIMEOUTS_MS, ini_file_full_path);
            device->data_timeout_ms = data_timeout_ms;
 
            if ( device->GetSensor() )
            {
                int max_alarms_number = ::GetPrivateProfileInt(current_sect, DEVICE_MAX_ALARMS_KEY, -1, ini_file_full_path);

                if ( max_alarms_number == -1 )
                    max_alarms_number = INT_MAX;

                max_number_of_sensor_alarms[i] = max_alarms_number;

                int max_time_in_alarm_mode = ::GetPrivateProfileInt(current_sect, DEVICE_MAX_TIME_IN_ALARM_STATE_KEY, -1, ini_file_full_path);

                sensors_max_times_in_alarm_mode[i] = DWORD(max_time_in_alarm_mode);    
            }

            devices.push_back(device);
        }
        else
            ret = false;
    }

    return ret;
}

static void CheckAndResize(std::vector<SExecuterParameters>& exec_parameters, int cur_param_ind)
{
    int params_size = exec_parameters.size();

    if ( cur_param_ind >= params_size )
    {
        SExecuterParameters last_params = exec_parameters[params_size - 1];
        exec_parameters.push_back( last_params );
    }
}

bool CRemoteDevicesManager::InitConnections(const char* ini_file_full_path)
{
    bool ret = true;

    char current_sect[30];
    char current_key[30];
    char output_buffer[1024];

    int num_connections = ::GetPrivateProfileInt(DEVICE_CONNECTIONS_SECT, DEVICE_CONNECTIONS_NUM_KEY, 0, ini_file_full_path);

    for ( int i = 0; (i < num_connections) && ret; i++ )
    {
        sprintf(current_sect, DEVICE_CONNECTION_SECT_PATTERN, i);

        int executer_ordinal = ::GetPrivateProfileInt(current_sect, DEVICE_CONNECTIONS_EXECUTER_KEY, -1, ini_file_full_path);

        if ( (executer_ordinal >= 0) && (executer_ordinal < devices.size()) )
        {
            ::GetPrivateProfileString(current_sect, DEVICE_CONNECTIONS_SENSORS_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);

            std::istrstream sensors_stream(output_buffer);
            int sensors_num;

            sensors_stream >> sensors_num;

            if ( sensors_num > 0)
            {
                std::vector<SSensorExecutionEvent> sensor_events_for_executer;
                char event_buffer[30];

                for ( int j = 0; (j < sensors_num) && ret; j++ )
                {
                    int sensor_ordinal;
                    sensors_stream >> sensor_ordinal;

                    if ( (sensor_ordinal >= 0) && (sensor_ordinal < devices.size()) && devices[sensor_ordinal]->GetSensor() )
                    {
                        event_buffer[0] = 0x0;
                        sensors_stream >> event_buffer;

                        ESensorEvent sensor_event = GetSensorEvent(event_buffer);

                        if ( sensor_event == LAST_SENSOR_EVENT )
                            ret = false;
                        else
                            sensor_events_for_executer.push_back( SSensorExecutionEvent(sensor_ordinal, sensor_event) );
                    }
                    else
                        ret = false;
                }

                if ( ret )
                {
                    std::vector<SExecuterParameters> exec_parameters;
                    exec_parameters.push_back( SExecuterParameters() );

                    for ( int j = 0; (j < LEVEL_BOUND) && ret; j++ )
                    {
                        int cur_param_ind = 0;

                        sprintf(current_key, DEVICE_CONNECTIONS_EXECUTER_PARAM_KEY_PATTERN, j);
                        ::GetPrivateProfileString(current_sect, current_key, "", output_buffer, sizeof(output_buffer), ini_file_full_path);

                        if ( output_buffer[0] != 0x0 )
                        {
                            char* params_buffer = output_buffer;
                            char* delim_pos = NULL;

                            while ( delim_pos = strchr(params_buffer, '!') )
                            {
                                char* delim_pos2 = strchr(delim_pos + 1, '!');

                                if ( delim_pos2 == NULL )
                                {
                                    ret = false;
                                    break;
                                }
                                else
                                {
                                    *delim_pos2 = 0x0;

                                    CheckAndResize(exec_parameters, cur_param_ind);

                                    for ( int k = cur_param_ind; k < exec_parameters.size(); k++ )
                                        exec_parameters[k].executer_parameters[j] = delim_pos + 1;

                                    cur_param_ind++;
                                    params_buffer = delim_pos2 + 1;
                                }
                            }
                        }
                    }

                    if ( ret )
                    {
                        ::GetPrivateProfileString(current_sect, DEVICE_SHOULD_REDO_IF_DEACTIVATED_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);

                        int cur_param_ind = 0;

                        if ( output_buffer[0] != 0x0 )
                        {
                            std::istrstream redo_flag_stream(output_buffer);

                            while( !redo_flag_stream.eof() )
                            {
                                bool should_redo;
                                redo_flag_stream >> should_redo;

                                CheckAndResize(exec_parameters, cur_param_ind);

                                for ( int k = cur_param_ind; k < exec_parameters.size(); k++ )
                                    exec_parameters[k].should_redo_if_executer_was_deactivated = should_redo;

                                cur_param_ind++;
                            }
                        }

                        ::GetPrivateProfileString(current_sect, DEVICE_SHOULD_LOG_EXECUTION_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);

                        cur_param_ind = 0;

                        if ( output_buffer[0] != 0x0 )
                        {
                            std::istrstream log_flag_stream(output_buffer);

                            while( !log_flag_stream.eof() )
                            {
                                bool should_log;
                                log_flag_stream >> should_log;

                                CheckAndResize(exec_parameters, cur_param_ind);

                                for ( int k = cur_param_ind; k < exec_parameters.size(); k++ )
                                    exec_parameters[k].should_log_action_execution = should_log;

                                cur_param_ind++;
                            }
                        }

                        ::GetPrivateProfileString(current_sect, DEVICE_TRANSFORM_PARAMETER_ON_EXECUTION_TIME_KEY, "", output_buffer, sizeof(output_buffer), ini_file_full_path);

                        cur_param_ind = 0;

                        if ( output_buffer[0] != 0x0 )
                        {
                            std::istrstream transform_flag_stream(output_buffer);

                            while( !transform_flag_stream.eof() )
                            {
                                bool should_transform;
                                transform_flag_stream >> should_transform;

                                CheckAndResize(exec_parameters, cur_param_ind);

                                for ( int k = cur_param_ind; k < exec_parameters.size(); k++ )
                                    exec_parameters[k].should_transform_parameter_on_execution_time = should_transform;

                                cur_param_ind++;
                            }
                        }
                        
                        SSensorsConnection sensors_connection;

                        sensors_connection.sensor_events_for_executer = sensor_events_for_executer;
                        sensors_connection.executer_ordinal_number = executer_ordinal;
                        sensors_connection.exec_params = exec_parameters;

                        sensor_connections.push_back(sensors_connection);
                    }
                }
            }
            else
                ret = false;

        }
        else
            ret = false;
    }

    if ( ret )
    {
        connection_processings_count = new int[sensor_connections.size()];

        for ( int i = 0; i < sensor_connections.size(); i++ )
            connection_processings_count[i] = 0;
    }

    return ret;
}

bool CRemoteDevicesManager::Init(const char* ini_file_full_path, ptr_Device_Creator remote_device_factory)
{
    bool ret = false;
    Clear();

    int num_devices = ::GetPrivateProfileInt(DEVICE_SECT, DEVICE_NUM_KEY, 0, ini_file_full_path);

    if ( num_devices > 0 )
    {
        max_number_of_sensor_alarms = new int[num_devices];
        sensor_alarms_count = new int[num_devices];
        sensors_max_times_in_alarm_mode = new DWORD[num_devices];
        sensors_start_alarm_times = new DWORD[num_devices];
        sensor_states = new ESensorState[num_devices];
        current_states = new ESensorState[num_devices];
        should_restore_sensor_states = true;
        int_get_sensor_state_attempts = 0;
        devices_execute_actions.clear();
        devices_saved_execution_parameters.resize(num_devices);
        current_device_index_to_activate = 0;
        
        for ( int i = 0; i < num_devices; i++ )
        {
            max_number_of_sensor_alarms[i] = INT_MAX;
            sensor_alarms_count[i] = 0;
            sensors_max_times_in_alarm_mode[i] = DWORD(-1);
            sensors_start_alarm_times[i] = DWORD(-1);
            sensor_states[i] = DEACTIVATED;
            current_states[i] = DEACTIVATED;
        }

        ret = InitDevices(num_devices, ini_file_full_path, remote_device_factory);        
        ret = ret && InitConnections(ini_file_full_path);
    }
   
    if ( !ret ) 
        Clear();

    return ret;
}

void CRemoteDevicesManager::SetSensorStatesActivated() const
{
    for ( int i = 0; i < devices.size(); i++ )
        sensor_states[i] = ACTIVATED;
}

void CRemoteDevicesManager::SetSensorStatesDeActivated() const
{
    for ( int i = 0; i < devices.size(); i++ )
        sensor_states[i] = DEACTIVATED;
}

bool CRemoteDevicesManager::IsAllDevicesActivated() const
{
    bool ret = true;

    for ( int i = 0; i < devices.size(); i++ )
    {
        ERemoteDeviceResponse device_response = devices[i]->IsActivated();
        
        if ( device_response != TRUE_RESPONSE )
        {
            ret = false;
            break;
        }
    }

    return ret;
}

ERemoteDeviceResponse CRemoteDevicesManager::IsObligatoryDevicesActivated() const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;

    for ( int i = 0; (i < devices.size()) && (ret != FALSE_RESPONSE); i++ )
    {
        if ( devices[i]->IsShouldBeAlwaysActivated() )
        {
            ERemoteDeviceResponse device_response = devices[i]->IsActivated();

            ret = ret && device_response;
        }
    }

    return ret;
}

short CRemoteDevicesManager::ActivateDevice(int device_ind) const
{
    short ret = OK;

    if ( device_ind < devices.size() )
    {
        bool should_perform_re_execution = false;
        bool should_activate_device = true;

        if ( devices[device_ind]->GetExecuter() && (devices_saved_execution_parameters[device_ind].size() > 0) )
        {
            ERemoteDeviceResponse device_response = devices[device_ind]->IsActivated();

            if ( device_response == FALSE_RESPONSE )
                should_perform_re_execution = true;
            else
            {
                if ( device_response == NO_RESPONSE )
                {
                    ret = ERROR_STATUS;

					if (main_server->IsExtendedLogging())
					{
						std::string msg = "НЕТ ОТВЕТА от устройства: " + devices[device_ind]->GetName();
						ptr_logger->LogMessage(msg, true);
					}
                }
                else
                    should_activate_device = false;
            }
        }

        if ( ret == OK )
        {
            if ( should_activate_device )
            {
                ERemoteDeviceResponse device_response = devices[device_ind]->Activate();

                if ( device_response != TRUE_RESPONSE )
                {
                    ret = ERROR_STATUS;

					if (main_server->IsExtendedLogging())
					{
						std::string msg = "Запрос Activate для устройства: " + devices[device_ind]->GetName();
						msg += " не выполнен - ";
						msg += GetResponseString(device_response);
						ptr_logger->LogMessage(msg, true);
					}
                }
            }

            if ( ret == OK )
            {
                if ( should_perform_re_execution )
                {
                    for ( int j = 0; j < devices_saved_execution_parameters[device_ind].size(); j++ )
                    {
                        SExecutionContext exec_context;

                        exec_context.executer_ordinal_number = device_ind;

                        for ( int k = 0; k < LEVEL_BOUND; k++ )
                            exec_context.exec_param.executer_parameters[k] = devices_saved_execution_parameters[device_ind][j];

                        exec_context.exec_param.should_redo_if_executer_was_deactivated = false;
                        exec_context.exec_param.should_log_action_execution = false;
                        exec_context.exec_param.should_transform_parameter_on_execution_time = true;

                        devices_execute_actions.push_back(exec_context);
                    }
                }
            }
        }
    }
    else
        ret = ERROR_STATUS;

    return ret;
}

short CRemoteDevicesManager::ActivateAllDevices() const
{
    short ret = OK;

    for ( int i = 0; i < devices.size(); i++ )
    {
        if ( ActivateDevice(i) != OK )
            ret = ERROR_STATUS;
    }

    return ret;
}

short CRemoteDevicesManager::ActivateNextDevice() const
{
    if ( current_device_index_to_activate >= devices.size() )
        current_device_index_to_activate = 0;

    short ret = ActivateDevice(current_device_index_to_activate);

    current_device_index_to_activate++;

    return ret;
}

short CRemoteDevicesManager::DeActivateAllDevices() const
{
    short ret = OK;

    for ( int i = 0; i < devices.size(); i++ )
    {
        ERemoteDeviceResponse device_response = devices[i]->DeActivate();
        
        if ( device_response != TRUE_RESPONSE )
        {
			if (main_server->IsExtendedLogging())
			{
				std::string msg = "Запрос DeActivate для устройства: " + devices[i]->GetName();
				msg += " не выполнен - ";
				msg += GetResponseString(device_response);
				ptr_logger->LogMessage(msg, true);
			}
         
            ret = ERROR_STATUS;
        }

        if ( devices[i]->IsSensorInEmulatedState() || devices[i]->IsExecuterInEmulatedState() )
        {
            devices[i]->StopEmulation();

            std::string msg = "Остановлена эмуляция устройства: " + devices[i]->GetName();
            ptr_logger->LogMessage(msg, true);

			/////////////////////////////////////////////////////////////////////////////////
			devices[i]->DeActivate();
			/////////////////////////////////////////////////////////////////////////////////
		}
    }

    return ret;
}

short CRemoteDevicesManager::RestoreSensorStates() const
{
    for ( int i = 0; i < devices.size(); i++ )
        current_states[i] = DEACTIVATED;

    int_get_sensor_state_attempts = 0;
    
    short ret = ActivateAllDevices();

    if ( ret == OK )
    {
        for ( int i = 0; i < devices.size(); i++ )
        {
            if ( devices[i]->GetSensor() )
            {
                ERemoteDeviceResponse state_set = devices[i]->GetSensor()->SetSensorState(sensor_states[i]);
                
                if ( state_set != TRUE_RESPONSE )
                {
					if (main_server->IsExtendedLogging())
					{
						std::string msg = "Запрос SetSensorState для устройства: " + devices[i]->GetName();
						msg += " не выполнен - ";
						msg += GetResponseString(state_set);
						ptr_logger->LogMessage(msg, true);
					}

                    ret = ERROR_STATUS;
                    break;
                }
            }
        }
    }
    
    if ( ret != OK )
        ptr_logger->LogMessage("Восстановление состояния устройств НЕУДАЧНО!!!", true);
	else
		ptr_logger->LogMessage("Состояния устройств восстановлены!!!");

    return ret;
}

ERemoteDeviceResponse CRemoteDevicesManager::GetCurrentSensorStates() const
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
    
    for ( int i = 0; (i < devices.size()) && (ret != FALSE_RESPONSE); i++ )
    {
        if ( devices[i]->GetSensor() )
        {
			if ( (sensor_states[i] == DISARMED) || (sensor_states[i] == SLEEP) ) //if sensor disarmed or sleep then it does not perform listening
				current_states[i] = sensor_states[i];
			else
			{
                if (current_states[i].GetNormalized() <= ACTIVATED) //in order not to miss alarm or disarm
				{
					ERemoteDeviceResponse device_response = devices[i]->GetSensor()->GetSensorState(current_states[i]);
       
	     			if ( device_response != TRUE_RESPONSE )
                    {
						if (main_server->IsExtendedLogging())
						{
							std::string msg = "Запрос GetSensorState для устройства: " + devices[i]->GetName();
							msg += " не выполнен - ";
							msg += GetResponseString(device_response);
							ptr_logger->LogMessage(msg, true);
						}

                        if ( main_server->SkipSensorIfNoResponse() && (device_response == NO_RESPONSE) )
                        {
                            current_states[i] = sensor_states[i];
                            device_response = TRUE_RESPONSE;

                            std::string msg = "Нет ответа GetSensorState для устройства: " + devices[i]->GetName();
                            msg += ",  сенсор пропущен!!!";
                            ptr_logger->LogMessage(msg, true);
                        }
                    }

                    if ((device_response == TRUE_RESPONSE) && (current_states[i] == DEACTIVATED))
                    {
						if (main_server->IsExtendedLogging())
						{
							std::string msg = "Запрос GetSensorState для устройства: " + devices[i]->GetName();
							msg += " вернул DEACTIVATED";
							ptr_logger->LogMessage(msg, true);
						}

                        device_response = FALSE_RESPONSE;
                    }

					ret = ret && device_response;
				}
			}
        }
        else
            current_states[i] = ACTIVATED;

        DWORD delay_between_get_sensor_states = main_server->GetDelayBetweenGetSensorStates();
        
        if ( delay_between_get_sensor_states > 0 )
            ::Sleep(delay_between_get_sensor_states);
    }

	if ( ret == TRUE_RESPONSE )
	{
		bool sensor_states_changed = false;

		for (int i = 0; i < devices.size(); i++)
		{
			if (devices[i]->GetSensor())
			{
                if (current_states[i].GetRaw() != sensor_states[i].GetRaw())
				{
					sensor_states_changed = true;
					break;
				}
			}
		}

		if (sensor_states_changed)
			ret = IsObligatoryDevicesActivated();
	}

    return ret;
}

ESensorEvent CRemoteDevicesManager::CalculateSensorEvent(ESensorState cur_state, ESensorState prev_state)
{
    ESensorEvent ret = LAST_SENSOR_EVENT;
    
    if ( (cur_state != DEACTIVATED) && (prev_state != DEACTIVATED) )
    {
        if ( cur_state == ACTIVATED )
        {
            switch (prev_state.GetNormalized())
            {
            case ACTIVATED:
                ret = LISTENING_CONTINUE;
                break;
            case ALARMED:
                ret = ALARM_OFF;
                break;
            case DISARMED:
                ret = DISARM_OFF;
                break;
            case SLEEP:
                ret = SLEEP_OFF;
                break;
            }
        }
        else
        {
            if ( cur_state == ALARMED )
            {
                switch (prev_state.GetNormalized())
                {
                case ACTIVATED:
                    ret = ALARM_ON;
                    break;
                case ALARMED:
                    ret = ALARM_CONTINUE;
                    break;
                case DISARMED:
                    ret = ALARM_ON;
                    break;
                case SLEEP:
                    ret = ALARM_ON;
                    break;
                }
            }
            else
            {
                if ( cur_state == DISARMED )
                {
                    switch (prev_state.GetNormalized())
                    {
                    case ACTIVATED:
                        ret = DISARM_ON;
                        break;
                    case ALARMED:
                        ret = DISARM_ON;
                        break;
                    case DISARMED:
                        ret = DISARM_CONTINUE;
                        break;
                    case SLEEP:
                        ret = DISARM_ON;
                        break;
                    }
                }
                else
                {
                    if ( cur_state == SLEEP )
                    {
                        switch (prev_state.GetNormalized())
                        {
                        case ACTIVATED:
                            ret = SLEEP_ON;
                            break;
                        case ALARMED:
                            ret = SLEEP_ON;
                            break;
                        case DISARMED:
                            ret = SLEEP_ON;
                            break;
                        case SLEEP:
                            ret = SLEEP_CONTINUE;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

void CRemoteDevicesManager::ProcessConnections(const ESensorEvent* sensor_events) const
{
    for ( int i = 0; i < sensor_connections.size(); i++ )
    {
        const SSensorsConnection& sensor_connection = sensor_connections[i];
        
        bool should_execute_connection = true;

        for ( int j = 0; j < sensor_connection.sensor_events_for_executer.size(); j++ )
        {
            const SSensorExecutionEvent& sensor_exe_event = sensor_connection.sensor_events_for_executer[j];

            if ( sensor_exe_event.sensor_event != sensor_events[sensor_exe_event.sensor_ordinal_number] )
            {
                should_execute_connection = false;
                break;
            }
        }

        if ( should_execute_connection )
        {
            SExecutionContext exec_context;

            exec_context.executer_ordinal_number = sensor_connection.executer_ordinal_number;
            
            int params_index = ( (connection_processings_count[i] < sensor_connection.exec_params.size()) ? connection_processings_count[i] : (sensor_connection.exec_params.size() - 1) );
            
            exec_context.exec_param = sensor_connection.exec_params[params_index];
            
            if ( !exec_context.exec_param.should_transform_parameter_on_execution_time )
            {
                for ( unsigned level_ind = 0; level_ind < LEVEL_BOUND; level_ind++ )
                    exec_context.exec_param.executer_parameters[level_ind] = CRemoteExecuter::TransformExecuterParameter(exec_context.exec_param.executer_parameters[level_ind].c_str(), this);
            }

            devices_execute_actions.push_back(exec_context);
            
            if ( connection_processings_count[i] < INT_MAX )
                connection_processings_count[i]++;
        }
    }
}

void CRemoteDevicesManager::ProcessSensors(ELevel level, DWORD quest_number_of_get_sensor_states_attempts) const
{
    if ( !should_restore_sensor_states )
    {
        ERemoteDeviceResponse get_states_succeded = GetCurrentSensorStates();

        if ( get_states_succeded == TRUE_RESPONSE )
        {
            ESensorEvent* sensor_events = new ESensorEvent[devices.size()];
            
            for ( int i = 0; i < devices.size(); i++ )
            {
                if ( sensor_alarms_count[i] >= max_number_of_sensor_alarms[i] )
                    ((ESensorState*)current_states)[i] = SLEEP;

                if ( sensors_max_times_in_alarm_mode[i] != DWORD(-1) )
                {
                    if ( current_states[i] == ALARMED )
                    {
                        if ( sensors_start_alarm_times[i] == DWORD(-1) )
                            sensors_start_alarm_times[i] = ::GetTickCount();
                        else
                        {
                            if ( (::GetTickCount() - sensors_start_alarm_times[i]) > sensors_max_times_in_alarm_mode[i] )
                            {
                                ((ESensorState*)current_states)[i] = ACTIVATED;
                                sensors_start_alarm_times[i] = DWORD(-1);
                            }
                        }
                    }
                    else
                        sensors_start_alarm_times[i] = DWORD(-1);
                }

                sensor_events[i] = CalculateSensorEvent(current_states[i], sensor_states[i]);
            }

            for ( int i = 0; i < devices.size(); i++ )
            {
                if ( sensor_events[i] == ALARM_ON )
                    sensor_alarms_count[i]++;

                sensor_states[i] = current_states[i];
                current_states[i] = DEACTIVATED;
            }

            int_get_sensor_state_attempts = 0;

			ProcessConnections(sensor_events);

			delete[] sensor_events;
        }
        else
        {
            if ( get_states_succeded == FALSE_RESPONSE )
                should_restore_sensor_states = true;
            else //NO_RESPONSE
            {
                if ( ++int_get_sensor_state_attempts > quest_number_of_get_sensor_states_attempts )
                {
                    should_restore_sensor_states = true;

                    ptr_logger->LogMessage("ПРЕВЫШЕНО максимальное количество попыток получения состояния сенсоров!!!", true);
                }
            }
        }
    }
    else
    {
        should_restore_sensor_states = (RestoreSensorStates() != OK);
    }
}

void CRemoteDevicesManager::ProcessExecuters(ELevel level) const
{
    if ( devices_execute_actions.size() > 0 )
    {
        const SExecutionContext& exec_context = devices_execute_actions[0];

        ERemoteDeviceResponse res = devices[exec_context.executer_ordinal_number]->DoAction(this, exec_context.exec_param.executer_parameters[level].c_str());

        if ( res != TRUE_RESPONSE )
        {
            std::string msg = "НЕ СРАБОТАЛО исп. устр.: " + devices[exec_context.executer_ordinal_number]->GetName();
            msg += ", с пар.: " + exec_context.exec_param.executer_parameters[level];
            ptr_logger->LogMessage(msg, true);

            if ( res == FALSE_RESPONSE ) //probably device rebooted and now is deactivated
                ActivateDevice(exec_context.executer_ordinal_number); 
        }
        else
        {
            if ( exec_context.exec_param.should_redo_if_executer_was_deactivated )
                devices_saved_execution_parameters[exec_context.executer_ordinal_number].push_back(exec_context.exec_param.executer_parameters[level]);

            if ( exec_context.exec_param.should_log_action_execution )
            {
                std::string msg = "Сработало исп. устр.: " + devices[exec_context.executer_ordinal_number]->GetName();
                msg += ", с пар.: " + exec_context.exec_param.executer_parameters[level];
                
                bool in_emulated_state = devices[exec_context.executer_ordinal_number]->IsExecuterInEmulatedState();
                
                if ( in_emulated_state )
                    msg += " (режим эмуляции!!!)";

                ptr_logger->LogMessage(msg, in_emulated_state);
            }
            
            devices_execute_actions.erase( devices_execute_actions.begin( ) );

            ProcessExecuters(level);
        }
    }
}

short CRemoteDevicesManager::ActivateSystem(bool need_log_start_info) const
{
    SetSensorStatesActivated();

    should_restore_sensor_states = (RestoreSensorStates() != OK);

	if ( !should_restore_sensor_states )
	{
		if ( !IsAllDevicesActivated() )
			should_restore_sensor_states = true;
	}
	
	if ( !should_restore_sensor_states )
	{
        ptr_logger->LogMessage("Система АКТИВИРОВАНА!!!");

		if ( need_log_start_info )
        {
            ptr_logger->LogMessage("---------GAME START---------");
            ptr_logger->LogNumberWithPattern(main_server->GetPlayersNum(), "PLAYERS NUMBER: %u");
        }
	}
    else
        ptr_logger->LogMessage("Активация НЕУДАЧНА!!!", true);

    return (should_restore_sensor_states ? ERROR_STATUS : OK);
}

short CRemoteDevicesManager::DeActivateSystem() const
{
    short ret = DeActivateAllDevices();

    SetSensorStatesDeActivated();

    for ( int i = 0; i < devices.size(); i++ )
    {
        sensor_alarms_count[i] = 0;
        sensors_start_alarm_times[i] = DWORD(-1);
    }

    for ( int i = 0; i < sensor_connections.size(); i++ )
        connection_processings_count[i] = 0;

    devices_execute_actions.clear();
    
    for ( int i = 0; i < devices.size(); i++ )
        devices_saved_execution_parameters[i].clear();

	current_device_index_to_activate = 0;
    
    if ( ret == OK )
		ptr_logger->LogMessage("Система ДЕАКТИВИРОВАНА!!!");
    else
        ptr_logger->LogMessage("Деактивация НЕУДАЧНА!!!", true);

    return ret;
}

short CRemoteDevicesManager::CheckSystem() const
{
    short ret = OK;

	DWORD start_time = ::GetTickCount();
	
	DWORD max_device_time = 0;
	int max_time_device_ind = 0;
	
	for ( int i = 0; i < devices.size(); i++ )
    {
		DWORD start_device_time = ::GetTickCount();
		
		ERemoteDeviceResponse device_response = devices[i]->Check();

		DWORD total_device_time = ::GetTickCount() - start_device_time;

		if (total_device_time > max_device_time)
		{
			max_device_time = total_device_time;
			max_time_device_ind = i;
		}
        
        if ( device_response != TRUE_RESPONSE )
        {
            std::string msg;
            
            if ( device_response == FALSE_RESPONSE )
                msg = "НЕ ПРОШЛО ПРОВЕРКУ устройство: " + devices[i]->GetName();
            else
                msg = "НЕТ ответа от устройства: " + devices[i]->GetName();

			ptr_logger->LogMessage(msg, true);
			
			ret = ERROR_STATUS;
        }
    }

	DWORD total_time = ::GetTickCount() - start_time;
	
	if ( ret == OK )
        ptr_logger->LogMessage("Проверка блока сценариев УДАЧНА!!!");
    else
        ptr_logger->LogMessage("Проверка блока сценариев НЕУДАЧНА!!!", true);

	ptr_logger->LogNumberWithPattern(devices.size(), "Проверено устройств: %u");
	
	char message_buf[1024];

	sprintf( message_buf, "Общее время проверки: %u (ms), максимальное время проверки одного устр-ва: %u (ms) для устр-ва: %s",
		     total_time, max_device_time, devices[max_time_device_ind]->GetName().c_str() );

	ptr_logger->LogMessage(message_buf);
	
	return ret;
}


