#include "stdafx.h"

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"

ERemoteDeviceResponse CRemoteSensor::GetSensorState(ESensorState& state) const
{
    if ( forcibly_set_state == LAST_STATE )
    {
        ERemoteDeviceResponse ret = NO_RESPONSE;
        
        for ( unsigned cnt = 0; (cnt < CRemoteDevice::GetMaxNumOfRequestsIfNoResponse()) && (ret == NO_RESPONSE); cnt++ )
            ret = GetSensorState_internal(state);

        return ret;
    }
    else
    {
        state = forcibly_set_state;
        return TRUE_RESPONSE;
    }
}

ERemoteDeviceResponse CRemoteSensor::SetSensorState(ESensorState state)
{
    if ( forcibly_set_state == LAST_STATE )
    {
        ERemoteDeviceResponse ret = NO_RESPONSE;

        for ( unsigned cnt = 0; (cnt < CRemoteDevice::GetMaxNumOfRequestsIfNoResponse()) && (ret == NO_RESPONSE); cnt++ )
            ret = SetSensorState_internal(state);

        return ret;
    }
    else
    {
        forcibly_set_state = state;
        return TRUE_RESPONSE;
    }
}
