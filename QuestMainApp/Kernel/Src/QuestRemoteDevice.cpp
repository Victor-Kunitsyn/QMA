#include "stdafx.h"

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"
#include "QuestRemoteExecuter.h"
#include "QuestScenariosManager.h"

const unsigned CRemoteDevice::max_num_of_requests_if_no_response = 3;


CRemoteDevice::CRemoteDevice( const std::string& ip, int port, const std::string& usr, const std::string& psw,
							  int id, int min_req_time_ms, const std::string& name ) : device_ip(ip),
																					   device_port(port),
																					   user(usr),
																					   password(psw),
                                                                                       device_id(id), 
                                                                                       min_time_ms_between_requests(min_req_time_ms),
                                                                                       device_name(name),
																					   should_be_always_activated(false),                                                                                       
                                                                                       connection_timeout_ms(DEFAULT_CURL_TIMEOUTS_MS),
                                                                                       data_timeout_ms(DEFAULT_CURL_TIMEOUTS_MS),
                                                                                       last_request_time(-1)
{
}

CRemoteDevice::~CRemoteDevice() 
{
}

void CRemoteDevice::CheckRequestTimeTreshold() const
{
    device_cs.Enter();

    DWORD current_time = ::GetTickCount();
    
    if ( (last_request_time != DWORD(-1)) && ((current_time - last_request_time) < min_time_ms_between_requests) )
    {
        ::Sleep( min_time_ms_between_requests - (current_time - last_request_time) );
    }
    
    last_request_time = ::GetTickCount();

    device_cs.Leave();
}

bool CRemoteDevice::IsSensorInEmulatedState() const
{
	bool ret = false;

	if ( GetSensor() )
		ret = GetSensor()->IsStateForced();

	return ret;
}

bool CRemoteDevice::IsExecuterInEmulatedState() const
{
	bool ret = false;

	if ( GetExecuter() )
		ret = GetExecuter()->IsDeviceDisregardered();

	return ret;
}

void CRemoteDevice::StopEmulation()
{
    if ( GetSensor() )
        GetSensor()->UnForceState(false);

	if ( GetExecuter() )
		GetExecuter()->RegardDevice();
}

static const char REMOTE_SENSOR_FORCE_STATE_PARAMETER[] = "FORCE_SENSOR_STATE ";
static const char REMOTE_SENSOR_UNFORCE_STATE_PARAMETER[] = "UNFORCE_SENSOR_STATE";

static const char REMOTE_EXECUTER_DISREGARD_PARAMETER[] = "DISREGARD_EXECUTER";
static const char REMOTE_EXECUTER_REGARD_PARAMETER[] = "REGARD_EXECUTER";

static const char REMOTE_DEVICE_ACTIVATE_PARAMETER[] = "DEVICE_ACTIVATE";
static const char REMOTE_DEVICE_DEACTIVATE_PARAMETER[] = "DEVICE_DEACTIVATE";

ERemoteDeviceResponse CRemoteDevice::DoAction(const CRemoteDevicesManager* ptr_device_mgr, const void* param /*= NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    CRemoteSensor* sensor = GetSensor();
    CRemoteExecuter* executer = GetExecuter();

    if ( param == NULL )
    {
        if ( executer )
            ret = executer->DoAction(param, ptr_device_mgr);
    }
    else
    {
        if ( strcmp((const char*)param, REMOTE_DEVICE_ACTIVATE_PARAMETER) == 0 )
            ret = Activate();
        else
        {
            if ( strcmp((const char*)param, REMOTE_DEVICE_DEACTIVATE_PARAMETER) == 0 )
                ret = DeActivate();
            else
            {
                if ( strcmp((const char*)param, REMOTE_SENSOR_UNFORCE_STATE_PARAMETER) == 0 )
                {
                    if ( sensor )
                    {
                        sensor->UnForceState(true);
                        ret = TRUE_RESPONSE;
                    }
                }
                else
                {
                    if ( strncmp((const char*)param, REMOTE_SENSOR_FORCE_STATE_PARAMETER, sizeof(REMOTE_SENSOR_FORCE_STATE_PARAMETER) - 1) == 0 )
                    {
                        if ( sensor )
                        {
                            ESensorState state = GetSensorState((const char*)param + sizeof(REMOTE_SENSOR_FORCE_STATE_PARAMETER) - 1);

                            if ( state != LAST_STATE )
                            {
                                sensor->ForceState(state);
                                ret = TRUE_RESPONSE;
                            }
                        }
                    }
                    else
                    {
                        if ( strcmp((const char*)param, REMOTE_EXECUTER_DISREGARD_PARAMETER) == 0 )
                        {
                            if ( executer )
                            {
                                executer->DisregardDevice();
                                ret = TRUE_RESPONSE;
                            }
                        }
                        else
                        {
                            if ( strcmp((const char*)param, REMOTE_EXECUTER_REGARD_PARAMETER) == 0 )
                            {
                                if ( executer )
                                {
                                    executer->RegardDevice();
                                    ret = TRUE_RESPONSE;
                                }
                            }
                            else
                            {
                                if ( executer )
                                    ret = executer->DoAction(param, ptr_device_mgr);
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

ERemoteDeviceResponse CRemoteDevice::Check() const
{
    CheckRequestTimeTreshold();
    
    ERemoteDeviceResponse ret = NO_RESPONSE;
    
    for ( unsigned cnt = 0; (cnt < max_num_of_requests_if_no_response) && (ret == NO_RESPONSE); cnt++ )
        ret = Check_internal();

    return ret;
}

ERemoteDeviceResponse CRemoteDevice::Activate() const
{
    CheckRequestTimeTreshold();
    
	ERemoteDeviceResponse ret = NO_RESPONSE;
	
    for ( unsigned cnt = 0; (cnt < max_num_of_requests_if_no_response) && (ret == NO_RESPONSE); cnt++ )
    {
        ret = TRUE_RESPONSE;

        bool real_activation_called = false;

        if ( GetSensor() )
        {
            if ( IsSensorInEmulatedState() )
            {
                ESensorState sensor_state;
                GetSensor()->GetSensorState(sensor_state);

                if (sensor_state == DEACTIVATED)
                    GetSensor()->SetSensorState(ACTIVATED);                
            }
            else
            {
                ret = Activate_internal();
                real_activation_called = true;
            }
        }

        if ( GetExecuter() )
        {
            if ( !IsExecuterInEmulatedState() && !real_activation_called )
                ret = ret && Activate_internal();
        }
    }

	return ret;	
}

ERemoteDeviceResponse CRemoteDevice::DeActivate() const
{
    CheckRequestTimeTreshold();

	ERemoteDeviceResponse ret = NO_RESPONSE;

    for ( unsigned cnt = 0; (cnt < max_num_of_requests_if_no_response) && (ret == NO_RESPONSE); cnt++ )
    {
        ret = TRUE_RESPONSE;

        bool real_deactivation_called = false;

        if ( GetSensor() )
        {
            if ( IsSensorInEmulatedState() )
                GetSensor()->SetSensorState(DEACTIVATED);
            else
            {
                ret = DeActivate_internal();
                real_deactivation_called = true;
            }
        }

        if ( GetExecuter() )
        {
            if ( !IsExecuterInEmulatedState() && !real_deactivation_called )
                ret = ret && DeActivate_internal();
        }
    }

	return ret;
}

ERemoteDeviceResponse CRemoteDevice::IsActivated() const
{
    CheckRequestTimeTreshold();

	ERemoteDeviceResponse ret = NO_RESPONSE;

    for ( unsigned cnt = 0; (cnt < max_num_of_requests_if_no_response) && (ret == NO_RESPONSE); cnt++ )
    {
        ret = TRUE_RESPONSE;

        bool real_is_activated_called = false;

        if ( GetSensor() )
        {
            if ( IsSensorInEmulatedState() )
            {
                ESensorState sensor_state;
                GetSensor()->GetSensorState(sensor_state);

                if (sensor_state == DEACTIVATED)
                    ret = FALSE_RESPONSE;
            }
            else
            {
                ret = IsActivated_internal();
                real_is_activated_called = true;
            }
        }

        if ( GetExecuter() )
        {
            if ( !IsExecuterInEmulatedState() && !real_is_activated_called )
                ret = ret && IsActivated_internal();
        }
    }

	return ret;	
}

CRemoteSensor* CRemoteDevice::GetSensor() const
{
    return dynamic_cast<CRemoteSensor*>((CRemoteDevice*)this);
}

CRemoteExecuter* CRemoteDevice::GetExecuter() const
{
    return dynamic_cast<CRemoteExecuter*>((CRemoteDevice*)this);
}
