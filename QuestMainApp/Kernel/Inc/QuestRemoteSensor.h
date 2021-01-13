#pragma once

/*
ESensorState enumeration value encodes 
sensor state (DEACTIVATED - SLEEP) and sensor value.

The rule for encoding is the following:
Final value = (LAST_STATE + 1) * (sensor value) + (sensor state)

For example, if sensor is in ALARM state and its value is 120,
then it returns (LAST_STATE + 1) * 120 + ALARM = 722

Last possible enumeration value is SENTINEL_STATE = 65535,
so sensor value can be from 0 up to 10921
*/

enum ESensorStateEnum
{
    DEACTIVATED = 0, //sensor deactivated, no listening
    ACTIVATED = 1, //sensor activated, listening performed
    ALARMED = 2, //sensor activated and alarmed
    DISARMED = 3, //sensor activated and disarmed
    SLEEP = 4, //sensor activated, but no listening performed (for example, sensor worked out its alarm limit)
    LAST_STATE = 5,

    SENTINEL_STATE = 65535
};

class ESensorState
{
public:
    ESensorState() : state(LAST_STATE) {}
    ESensorState(ESensorStateEnum state_p) : state(state_p) {}
    
    ~ESensorState() {}

    inline unsigned GetValue() const
    {
        return (unsigned)(double(state) / (LAST_STATE + 1));
    }

    inline ESensorStateEnum GetNormalized() const
    {
        return (ESensorStateEnum)(state - GetValue() * (LAST_STATE + 1));
    }

    inline ESensorStateEnum GetRaw() const
    {
        return state;
    }

private:
    ESensorStateEnum state;
};

inline bool operator == (const ESensorState& one, const ESensorState& two)
{
    return ( one.GetNormalized() == two.GetNormalized() );
}

inline bool operator != (const ESensorState& one, const ESensorState& two)
{
    return !(one == two);
}

inline ESensorState EncodeSensorState(ESensorStateEnum state, unsigned value)
{
    return (ESensorStateEnum)((LAST_STATE + 1) * value + state);
}

class CRemoteSensor
{
public:
    CRemoteSensor() : forcibly_set_state(LAST_STATE) {}
    virtual ~CRemoteSensor() {}

public:
    ERemoteDeviceResponse GetSensorState(ESensorState& state) const; 
    ERemoteDeviceResponse SetSensorState(ESensorState state); 

    void ForceState(ESensorState state) { forcibly_set_state = state; }
    
    void UnForceState(bool should_set_activated_state) 
    { 
        forcibly_set_state = LAST_STATE; 
        
        if ( should_set_activated_state )
            SetSensorState(ACTIVATED); 
    }
    
    bool IsStateForced() const { return (forcibly_set_state != LAST_STATE); }

protected:
    virtual ERemoteDeviceResponse GetSensorState_internal(ESensorState& state) const = 0; //must return DEACTIVATED if device is not active
    virtual ERemoteDeviceResponse SetSensorState_internal(ESensorState state) = 0; //must activate device if state != DEACTIVATED

protected:
    ESensorState forcibly_set_state; //if forcibly_set_state != LAST_STATE (i.e. set) then real sensor is not monitored.
                                     //it can be useful, if, for example, real sensor device is broken and does not respond at all

    friend class CRemoteDevicesManager;
};