#pragma once

class CRemoteDevicesManager;

class CRemoteExecuter 
{
public:
    CRemoteExecuter() : is_device_forcibly_disregardered(false) {}
    virtual ~CRemoteExecuter() {}

public:
    ERemoteDeviceResponse DoAction(const void* param = NULL, const CRemoteDevicesManager* ptr_device_mgr = NULL) const;

    void DisregardDevice() { is_device_forcibly_disregardered = true; }
    void RegardDevice() { is_device_forcibly_disregardered = false; }
    bool IsDeviceDisregardered() const { return is_device_forcibly_disregardered; }

public:    
    static std::string TransformExecuterParameter(const void* param, const CRemoteDevicesManager* ptr_device_mgr);

protected:
    virtual ERemoteDeviceResponse DoAction_internal(const void* param = NULL) const = 0; //must do nothing and return false if device is not activated

protected:
    bool is_device_forcibly_disregardered; //if this flag set to true, then DoAction method does nothing and returns true
                                           //it can be useful, if, for example, real executer device is broken and does not respond at all  

};