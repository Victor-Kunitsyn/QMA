#pragma once

#include "memStream.h"

enum ESecurityCheckResult
{
    SEC_CODE_OK = 0,                //OK, security code has been accepted
    SEC_CODE_INVALID = 1,           //Invalid security code
    SEC_CODE_EXPIRED = 2,           //Security code has been expired 
    SEC_CODE_BLOCKED = 3,           //Security code has been blocked by SU 
    SEC_CODE_SHARING_VIOLATION = 4  //Security code is already in use by another application copy
};

ESecurityCheckResult CheckSecurityCode(const char* sec_code, memStream& mem_stream, bool& super_user_security_code, bool& code_can_be_fixed, bool& code_is_fixed);

bool ReleaseSecurityCode(memStream& mem_stream);

bool MountSecurityCode(memStream& mem_stream);

bool UnMountSecurityCode(memStream& mem_stream);

DWORD GetPhysicalDriveSerialNumber(UINT nDriveNumber, std::string& strSerialNumber);

DWORD GetHash(const std::string& str);