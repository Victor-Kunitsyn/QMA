#pragma once

#include <vector>

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"
#include "QuestRemoteExecuter.h"

//Special build-in Kernel executer/sensor to get into alarm state, when some predefined time condition has been met
//Its type should be marked by TIME_CONDITIONAL_BUILD_IN_DEVICE_TYPE in Quest *.ini file

#define TIME_CONDITIONAL_BUILD_IN_DEVICE_TYPE "TIME_CONDITIONAL_BUILD_IN_DEVICE"

class CMainServer;

class CQuestTimeConditionalDevice : public CRemoteDevice, public CRemoteExecuter, public CRemoteSensor 
{
public:
	// parameters describing the times when sensor should get into alarm state are transfered via string "usr"
    // parameter whether this timer starts on quest start or on demand is transfered via string "psw"
    CQuestTimeConditionalDevice(const std::string& ip, int port, const std::string& usr, const std::string& psw,
		                        int id, int request_threshold, const std::string& name);

	virtual ~CQuestTimeConditionalDevice();

    void SetOwner(const CMainServer* main_server_ptr) { main_server = main_server_ptr; }

protected:
	virtual ERemoteDeviceResponse Check_internal() const;
	virtual ERemoteDeviceResponse Activate_internal() const;
	virtual ERemoteDeviceResponse DeActivate_internal() const;
	virtual ERemoteDeviceResponse IsActivated_internal() const;

protected:
    virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const;

protected:
    virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const;
    virtual ERemoteDeviceResponse SetSensorState_internal(ESensorState state);

private:
	void ClearTimer() const;

protected:
    const CMainServer* main_server;

    mutable bool is_activated;   

    std::vector<DWORD> times_past_timer_start;
    DWORD time_step;
    bool start_on_quest_start;
    
	mutable bool timer_stopped;
	mutable DWORD timer_start_time;

    mutable int last_alarm_ind; //index in times_past_timer_start vector
    mutable DWORD last_num_timer_steps;
};

