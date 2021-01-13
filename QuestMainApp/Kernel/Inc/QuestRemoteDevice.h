#pragma once

#include <string>
#include "QuestCommon.h"

#define DEFAULT_CURL_TIMEOUTS_MS 1000

class CRemoteSensor;
class CRemoteExecuter;
class CRemoteDevicesManager;

enum ERemoteDeviceResponse //order of this enumeration is important!!!
{
    FALSE_RESPONSE = 0,
    NO_RESPONSE = 1, //for example, device' WiFi module does not respond due to connection lost
    TRUE_RESPONSE = 2
};

inline ERemoteDeviceResponse operator && (ERemoteDeviceResponse resp1, ERemoteDeviceResponse resp2)
{
    return min(resp1, resp2);
}

class CRemoteDevice
{
public:
	CRemoteDevice( const std::string& ip, int port, const std::string& usr, const std::string& psw,  
		           int id, int min_req_time_ms, const std::string& name );
    
	virtual ~CRemoteDevice();

public:
    const std::string& GetName() const { return device_name; }

public:
    ERemoteDeviceResponse Check() const;
    ERemoteDeviceResponse Activate() const;
    ERemoteDeviceResponse DeActivate() const;
    ERemoteDeviceResponse IsActivated() const;

    CRemoteSensor* GetSensor() const;
    CRemoteExecuter* GetExecuter() const;

    bool IsSensorInEmulatedState() const;
	bool IsExecuterInEmulatedState() const;

public:
    ERemoteDeviceResponse DoAction(const CRemoteDevicesManager* ptr_device_mgr, const void* param = NULL) const;

public:
    static unsigned GetMaxNumOfRequestsIfNoResponse() { return max_num_of_requests_if_no_response; }

protected:
    virtual ERemoteDeviceResponse Check_internal() const = 0;
    virtual ERemoteDeviceResponse Activate_internal() const = 0; //must do nothing if device has been already activated
    virtual ERemoteDeviceResponse DeActivate_internal() const = 0;
    virtual ERemoteDeviceResponse IsActivated_internal() const = 0;

protected:
	bool IsShouldBeAlwaysActivated() const { return should_be_always_activated; }
    void CheckRequestTimeTreshold() const;
    void StopEmulation();

protected:
    std::string device_ip;
	int device_port;
	std::string user;
	std::string password;
    
	int device_id; //device ordering number in set of devices connected to the same ip address
    int min_time_ms_between_requests;
    std::string device_name;

	bool should_be_always_activated;

    unsigned long connection_timeout_ms;
    unsigned long data_timeout_ms;
    
    mutable CScopedCriticalSection device_cs;
    mutable DWORD last_request_time;

	static const unsigned max_num_of_requests_if_no_response;
    
    friend class CRemoteDevicesManager;
};