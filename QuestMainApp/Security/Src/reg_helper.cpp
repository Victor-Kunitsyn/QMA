#include "stdafx.h"

#include <atlbase.h>

#include "reg_helper.h"

const std::string REG_SW_GROUP = "Software\\Quest Control Application\\QuestMainApp\\Security";
const std::string REG_KEY = "HDDSerialHash";

DWORD ReadRegistryValue()
{
    DWORD ret = 0;

    CRegKey regKey;

    if( regKey.Open(HKEY_LOCAL_MACHINE, REG_SW_GROUP.c_str()) == ERROR_SUCCESS )
    {
        if( regKey.QueryDWORDValue(REG_KEY.c_str(), ret) != ERROR_SUCCESS )
            ret = 0;
    }

    regKey.Close();

    return ret;
}

bool WriteRegistryValue(DWORD value)
{
    bool ret = false;

    CRegKey regKey;

    if( regKey.Create(HKEY_LOCAL_MACHINE, REG_SW_GROUP.c_str()) == ERROR_SUCCESS )
    {
        if( regKey.SetDWORDValue(REG_KEY.c_str(), value) == ERROR_SUCCESS )
        {
            regKey.Flush();
            ret = true;
        }
    }

    regKey.Close();

    return ret;
}

bool DeleteRegistryValue()
{
    bool ret = false;

    CRegKey regKey;

    if( regKey.Open(HKEY_LOCAL_MACHINE, REG_SW_GROUP.c_str()) == ERROR_SUCCESS )
    {
        if( regKey.DeleteValue(REG_KEY.c_str()) == ERROR_SUCCESS )
        {
            regKey.Flush();
            ret = true;
        }
    }
    
    regKey.Close();

    return ret;
}

