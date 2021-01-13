#pragma once

#include "QuestRemoteDevice.h"
#include "QuestRemoteExecuter.h"
#include "QuestRemoteSensor.h"

class COpr;

class CTenvisIPCamExecuter: public CRemoteDevice, public CRemoteExecuter, public CRemoteSensor
{
public:
    enum ECameraMoveControl
    {
        UP = 0,
        STOP_UP = 1,
        DOWN = 2,
        STOP_DOWN = 3,
        LEFT = 4,
        STOP_LEFT = 5,
        RIGHT = 6,
        STOP_RIGHT = 7,
        CENTER = 25
    };

    static ECameraMoveControl GetStopMovingParameter(ECameraMoveControl move_param);

public:
    CTenvisIPCamExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
		                 int id, int request_threshold, const std::string& name);
    
	virtual ~CTenvisIPCamExecuter();

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
    static DWORD WINAPI ThreadProc(LPVOID lpParameter);

    short StartActivity() const;
    short FinishActivity() const;

	bool RebootCamera();
	bool Connect();
	bool DisConnect();

private:
	COpr* camera_handle;
	bool is_connected;
    mutable ESensorState current_state;

  	mutable HANDLE m_hThread;
    mutable DWORD m_dwThreadID;

    mutable std::string full_path_to_wav_file;
    mutable DWORD last_move_time;

    mutable DWORD move_counter;
    static const ECameraMoveControl camera_move_steps[];

    mutable CScopedCriticalSection camera_cs;
};