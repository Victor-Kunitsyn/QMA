#include "stdafx.h"
#include <strstream>

#include "QuestTimeDevice.h"
#include "QuestServer.h"

static const char TIMER_ON_QUEST_START_KEY[] = "QUEST START";
static const char TIMER_ON_DEMAND_START_KEY[] = "ON DEMAND";

static const char TIMER_START_EXECUTER_PARAMETER[] = "START";
static const char TIMER_STOP_EXECUTER_PARAMETER[] = "STOP";

CQuestTimeConditionalDevice::CQuestTimeConditionalDevice(const std::string& ip, int port, const std::string& usr, const std::string& psw,
                                                         int id, int request_threshold, const std::string& name) : 
                                                         CRemoteDevice(ip, port, "", "", id, request_threshold, name), 
                                                         main_server(NULL), is_activated(false), timer_stopped(false), timer_start_time(-1), last_alarm_ind(-1), last_num_timer_steps(0)
{
    start_on_quest_start = ( psw == TIMER_ON_QUEST_START_KEY );
    
    time_step = DWORD(-1);

    std::istrstream times_stream(usr.c_str());

    while( !times_stream.eof() )
    {
        times_stream >> time_step;
        
        if ( time_step != DWORD(-1) )
            times_past_timer_start.push_back(time_step);
        else
            break;
    }

    for ( int i = 1; i < times_past_timer_start.size(); i++ )
        times_past_timer_start[i] += times_past_timer_start[i - 1];
}

CQuestTimeConditionalDevice::~CQuestTimeConditionalDevice()
{
}

void CQuestTimeConditionalDevice::ClearTimer() const
{
	timer_start_time = DWORD(-1);
	last_alarm_ind = -1;
	last_num_timer_steps = 0;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::Check_internal() const
{
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::Activate_internal() const
{
    if ( !is_activated )
	{
		ClearTimer();
		timer_stopped = false;
        is_activated = true;
	}
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::DeActivate_internal() const
{
	ClearTimer();
	timer_stopped = false;
    is_activated = false;
    
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::IsActivated_internal() const
{
    return ( is_activated ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::DoAction_internal(const void* param /*= NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( is_activated && param )
    {
		if ( _stricmp((const char*)param, TIMER_START_EXECUTER_PARAMETER) == 0 )
		{
			if (!start_on_quest_start && (timer_start_time == DWORD(-1)))
			{
				timer_start_time = ::GetTickCount();
				timer_stopped = false;
				
				ret = TRUE_RESPONSE;
			}
		}
		else //stop timer
		{
			ClearTimer();
			timer_stopped = true;

			ret = TRUE_RESPONSE;
		}
    }

    return ret;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::GetSensorState_internal(ESensorState& state) const
{
    if ( !is_activated )
        state = DEACTIVATED;
    else
    {
        state = ACTIVATED;
        
		if ( !timer_stopped )
		{
			DWORD start_time = DWORD(-1);

			if (start_on_quest_start)
			{
				if (main_server)
					start_time = main_server->GetSystemActivationTime();
			}
			else
				start_time = timer_start_time;

			if ((start_time != DWORD(-1)) && (times_past_timer_start.size() > 0))
			{
				DWORD elapsed_time_ms = ::GetTickCount() - start_time;

				if (last_alarm_ind < int(times_past_timer_start.size()) - 1)
				{
					if (elapsed_time_ms >= times_past_timer_start[last_alarm_ind + 1])
					{
						state = ALARMED;
						last_alarm_ind++;
					}
				}
				else
				{
					if (time_step != DWORD(-1))
					{
						if (time_step != 0)
						{
							elapsed_time_ms -= times_past_timer_start[last_alarm_ind];

							DWORD num_timer_steps = elapsed_time_ms / time_step;

							if (num_timer_steps > last_num_timer_steps)
							{
								last_num_timer_steps = num_timer_steps;
								state = ALARMED;
							}
						}
						else
							state = ALARMED;
					}
					else
					{
						ClearTimer();
						timer_stopped = true;
					}
				}
			}
		}
    }
    
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CQuestTimeConditionalDevice::SetSensorState_internal(ESensorState state)
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
    
    if ( state != DEACTIVATED )
        ret = Activate();
	
	//nothing to do
    
    return ret;
}
