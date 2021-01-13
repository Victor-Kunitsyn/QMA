
#include "stdafx.h"

#include <winioctl.h>
#include <Ntddstor.h>

#include <string>
#include "check_security.h"
#include "reg_helper.h"

#define SEC_CODE_LEN 8

/*BlockedBySU 0
ExpirationDate 17.5.2020
LastCheckDate 9.7.2017
InUse 0
UserDiskSerial       RC250FCB2S7U6K
CanBeFixed 1
IsFixed 0
Info It is a test key*/

static const char BlockedBySUKey[] = "BlockedBySU";
static const char ExpirationDateKey[] = "ExpirationDate";
static const char LastCheckDateKey[] = "LastCheckDate";
static const char InUseKey[] = "InUse";
static const char UserDiskSerialKey[] = "UserDiskSerial";
static const char CanBeFixedKey[] = "CanBeFixed";
static const char IsFixedKey[] = "IsFixed";
static const char InfoKey[] = "Info";

class CSecurityFileAccessor
{
public:
	CSecurityFileAccessor();
	virtual ~CSecurityFileAccessor() {}

	bool InitByMemoryStream(const char* data);

	bool IsBlockedBySU() const { return blocked_by_su; }
	bool IsLicenseExpired(const SYSTEMTIME& cur_time) const;
	bool IsUsedByAnotherCopy() const;

	void UpdateUsageState(bool is_using);
	void UpdateLastCheckTime(const SYSTEMTIME& cur_time);

	void UpdateMemoryStream(memStream& mem_stream) const;

	bool CheckHash(DWORD hdd_hash) const;
	DWORD GetHash() const;

	bool CanBeFixed() const { return can_be_fixed; }
	bool IsFixed() const { return is_fixed; }

	bool MountCode();
	bool UnMountCode();

protected:
	void Clear();

private:
	bool blocked_by_su;

	unsigned long exp_day;
	unsigned long exp_month;
	unsigned long exp_year;

	unsigned long last_check_day;
	unsigned long last_check_month;
	unsigned long last_check_year;

	bool in_use;
	std::string user_disk_serial;

	bool can_be_fixed;
	bool is_fixed;
	std::string key_info;

	std::string user_this_disk_serial;
};

CSecurityFileAccessor::CSecurityFileAccessor() : user_this_disk_serial("")
{
	Clear();

	if (GetPhysicalDriveSerialNumber(0, user_this_disk_serial) != NO_ERROR)
		user_this_disk_serial = "";
}

void CSecurityFileAccessor::Clear()
{
	blocked_by_su = true;

	exp_day = 0;
	exp_month = 0;
	exp_year = 0;

	last_check_day = 0;
	last_check_month = 0;
	last_check_year = 0;

	in_use = true;
	user_disk_serial = "";

	can_be_fixed = false;
	is_fixed = false;
	key_info = "";
}

bool CSecurityFileAccessor::CheckHash(DWORD hdd_hash) const
{
	return (GetHash() == hdd_hash);
}

DWORD CSecurityFileAccessor::GetHash() const
{
	return ::GetHash(user_this_disk_serial);
}

static const char* GetValueByKey(const char* data, const char* key)
{
	const char* ret = NULL;

	const char* data_ptr = strstr(data, key);

	if (data_ptr)
	{
		data_ptr = data_ptr + strlen(key);

		if ((*data_ptr == ' ') && (*(data_ptr + 1) != 0x0))
		{
			ret = data_ptr + 1;
		}
	}

	return ret;
}

//converts dd.mm.yyyy string to day, month, year
bool RetrieveTime(const char* data_ptr, unsigned long& day, unsigned long& month, unsigned long& year)
{
	bool ret = false;

	day = 0;
	month = 0;
	year = 0;

	if (data_ptr)
	{
		if (strlen(data_ptr) >= sizeof("dd.mm.yyyy"))
		{
			char* term_char = NULL;
			day = strtoul(data_ptr, &term_char, 10);

			if (term_char && (*term_char == '.'))
			{
				month = strtoul(term_char + 1, &term_char, 10);

				if (term_char && (*term_char == '.'))
				{
					year = strtoul(term_char + 1, &term_char, 10);

					if (term_char && ((*term_char == 0x0) || (*term_char == 0x0D) || (*term_char == 0x0A)))
					{
						ret = true;
					}
				}
			}
		}
	}

	return ret;
}

bool CSecurityFileAccessor::InitByMemoryStream(const char* data)
{
	bool ret = false;

	Clear();

	const char* data_ptr = GetValueByKey(data, BlockedBySUKey);

	if (data_ptr)
	{
		blocked_by_su = (*data_ptr != '0');

		data_ptr = GetValueByKey(data, ExpirationDateKey);

		if (RetrieveTime(data_ptr, exp_day, exp_month, exp_year))
		{
			data_ptr = GetValueByKey(data, LastCheckDateKey);

			if (RetrieveTime(data_ptr, last_check_day, last_check_month, last_check_year))
			{
				data_ptr = GetValueByKey(data, InUseKey);

				if (data_ptr)
				{
					in_use = (*data_ptr != '0');

					data_ptr = GetValueByKey(data, UserDiskSerialKey);

					if (data_ptr)
					{
						size_t serial_end_index = strcspn(data_ptr, "\xD\xA");

						user_disk_serial = data_ptr;

						if (serial_end_index < strlen(data_ptr))
							user_disk_serial.erase(serial_end_index);

						data_ptr = GetValueByKey(data, CanBeFixedKey);

						if (data_ptr)
						{
							can_be_fixed = (*data_ptr != '0');

							data_ptr = GetValueByKey(data, IsFixedKey);

							if (data_ptr)
							{
								is_fixed = (*data_ptr != '0');
								
								data_ptr = GetValueByKey(data, InfoKey);

								if (data_ptr)
								{
									size_t info_end_index = strcspn(data_ptr, "\xD\xA");

									key_info = data_ptr;

									if (info_end_index < strlen(data_ptr))
										key_info.erase(info_end_index);

									ret = true;
								}
							}
						}
					}
				}
			}
		}
	}

	if (!ret)
		Clear();

	return ret;
}

static bool IsGreater(const SYSTEMTIME& t1, const SYSTEMTIME& t2)
{
	bool ret = false;

	if (t1.wYear > t2.wYear)
		ret = true;
	else
	{
		if (t1.wYear == t2.wYear)
		{
			if (t1.wMonth > t2.wMonth)
				ret = true;
			else
			{
				if (t1.wMonth == t2.wMonth)
				{
					if (t1.wDay > t2.wDay)
						ret = true;
				}
			}
		}
	}

	return ret;
}

bool CSecurityFileAccessor::IsLicenseExpired(const SYSTEMTIME& cur_time) const
{
	SYSTEMTIME expiry_time, last_check_time;

	expiry_time.wYear = exp_year;
	expiry_time.wMonth = exp_month;
	expiry_time.wDay = exp_day;

	last_check_time.wYear = last_check_year;
	last_check_time.wMonth = last_check_month;
	last_check_time.wDay = last_check_day;

	return (IsGreater(cur_time, expiry_time) || IsGreater(last_check_time, cur_time));
}

bool CSecurityFileAccessor::IsUsedByAnotherCopy() const
{
	return (in_use && (user_disk_serial != user_this_disk_serial));
}

bool CSecurityFileAccessor::MountCode()
{
	bool ret = false;

	if (WriteRegistryValue(GetHash()))
	{
		UpdateUsageState(true);

		can_be_fixed = false;
		is_fixed = true;

		ret = true;
	}

	return ret;
}

bool CSecurityFileAccessor::UnMountCode()
{
	bool ret = false;

	if (DeleteRegistryValue())
	{
		can_be_fixed = true;
		is_fixed = false;

		ret = true;
	}

	return ret;
}
void CSecurityFileAccessor::UpdateUsageState(bool is_using)
{
	in_use = is_using;
	user_disk_serial = user_this_disk_serial;
}

void CSecurityFileAccessor::UpdateLastCheckTime(const SYSTEMTIME& cur_time)
{
	last_check_day = cur_time.wDay;
	last_check_month = cur_time.wMonth;
	last_check_year = cur_time.wYear;
}

//converts day, month, year to dd.mm.yyyy string
static std::string ConvertToString(unsigned long day, unsigned long month, unsigned long year)
{
	std::string ret = "";

	char digit_str[255];

	_ultoa(day, digit_str, 10);
	ret += digit_str;
	ret += ".";
	_ultoa(month, digit_str, 10);
	ret += digit_str;
	ret += ".";
	_ultoa(year, digit_str, 10);
	ret += digit_str;

	return ret;
}

void CSecurityFileAccessor::UpdateMemoryStream(memStream& mem_stream) const
{
	memStreamCleanup(&mem_stream);

	std::string output = BlockedBySUKey;
	output += " ";
	output += (blocked_by_su ? "1" : "0");
	output += 0x0D; output += 0x0A;

	output += ExpirationDateKey;
	output += " ";
	output += ConvertToString(exp_day, exp_month, exp_year);
	output += 0x0D; output += 0x0A;

	output += LastCheckDateKey;
	output += " ";
	output += ConvertToString(last_check_day, last_check_month, last_check_year);
	output += 0x0D; output += 0x0A;

	output += InUseKey;
	output += " ";
	output += (in_use ? "1" : "0");
	output += 0x0D; output += 0x0A;

	output += UserDiskSerialKey;
	output += " ";
	output += user_disk_serial;
	output += 0x0D; output += 0x0A;

	output += CanBeFixedKey;
	output += " ";
	output += (can_be_fixed ? "1" : "0");
	output += 0x0D; output += 0x0A;

	output += IsFixedKey;
	output += " ";
	output += (is_fixed ? "1" : "0");
	output += 0x0D; output += 0x0A;

	output += InfoKey;
	output += " ";
	output += key_info;
	output += 0x0D; output += 0x0A;

	mem_stream.size = output.length() + 1;
	mem_stream.data = (char*)malloc(mem_stream.size);
	strcpy(mem_stream.data, output.c_str());
}

////////////////////////////////////////////////////////////////////////////////////

static const char super_user_sec_code[] = { 'b', 'd', 'h', 'j', 'n', 'r', 'S', 'U' };
static const int  super_user_sec_ind[] = { 1, 0, 5, 4, 3, 2, 6, 7 };

ESecurityCheckResult CheckSecurityCode(const char* sec_code, memStream& mem_stream, bool& super_user_security_code, bool& code_can_be_fixed, bool& code_is_fixed)
{
	ESecurityCheckResult ret = SEC_CODE_INVALID;

	super_user_security_code = false;
	code_can_be_fixed = false;
	code_is_fixed = false;

	if (strlen(sec_code) == SEC_CODE_LEN)
	{
		super_user_security_code = true;

		for (int i = 0; i < SEC_CODE_LEN; i++)
		{
			if (sec_code[i] != super_user_sec_code[super_user_sec_ind[i]])
			{
				super_user_security_code = false;
				break;
			}
		}

		if (super_user_security_code)
			ret = SEC_CODE_OK;
		else
		{
			CSecurityFileAccessor security_accessor;

			DWORD hdd_hash = ReadRegistryValue();

			if (security_accessor.CheckHash(hdd_hash)) //security code is mounted to this machine
			{
				code_is_fixed = true;
				ret = SEC_CODE_OK;
			}
			else
			{
				if (mem_stream.data && security_accessor.InitByMemoryStream(mem_stream.data))
				{
					if (security_accessor.IsBlockedBySU())
						ret = SEC_CODE_BLOCKED;
					else
					{
						SYSTEMTIME cur_time;
						::GetSystemTime(&cur_time);

						if (security_accessor.IsLicenseExpired(cur_time))
							ret = SEC_CODE_EXPIRED;
						else
						{
							if (security_accessor.IsUsedByAnotherCopy())
								ret = SEC_CODE_SHARING_VIOLATION;
							else
							{
								security_accessor.UpdateUsageState(true);
								security_accessor.UpdateLastCheckTime(cur_time);

								security_accessor.UpdateMemoryStream(mem_stream);

								code_can_be_fixed = security_accessor.CanBeFixed();

								ret = SEC_CODE_OK;
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

bool ReleaseSecurityCode(memStream& mem_stream)
{
	bool ret = false;

	CSecurityFileAccessor security_accessor;

	if (mem_stream.data && security_accessor.InitByMemoryStream(mem_stream.data))
	{
		if (!security_accessor.IsUsedByAnotherCopy() && !security_accessor.IsFixed())
		{
			security_accessor.UpdateUsageState(false);

			security_accessor.UpdateMemoryStream(mem_stream);

			ret = true;
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////

DWORD GetPhysicalDriveSerialNumber(UINT nDriveNumber, std::string& strSerialNumber)
{
	DWORD dwResult = NO_ERROR;
	strSerialNumber = "";

	// Format physical drive path (may be '\\.\PhysicalDrive0', '\\.\PhysicalDrive1' and so on).
	CString strDrivePath;
	strDrivePath.Format(_T("\\\\.\\PhysicalDrive%u"), nDriveNumber);

	// call CreateFile to get a handle to physical drive
	HANDLE hDevice = ::CreateFile(strDrivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
		return ::GetLastError();

	// set the input STORAGE_PROPERTY_QUERY data structure
	STORAGE_PROPERTY_QUERY storagePropertyQuery;
	ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
	storagePropertyQuery.PropertyId = StorageDeviceProperty;
	storagePropertyQuery.QueryType = PropertyStandardQuery;

	// get the necessary output buffer size
	STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
	DWORD dwBytesReturned = 0;
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
		&dwBytesReturned, NULL))
	{
		dwResult = ::GetLastError();
		::CloseHandle(hDevice);
		return dwResult;
	}

	// allocate the necessary memory for the output buffer
	const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
	BYTE* pOutBuffer = new BYTE[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);

	// get the storage device descriptor
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer, dwOutBufferSize,
		&dwBytesReturned, NULL))
	{
		dwResult = ::GetLastError();
		delete[]pOutBuffer;
		::CloseHandle(hDevice);
		return dwResult;
	}

	// Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
	// followed by additional info like vendor ID, product ID, serial number, and so on.
	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;
	if (dwSerialNumberOffset != 0)
	{
		// finally, get the serial number
		strSerialNumber = (const char*)(pOutBuffer + dwSerialNumberOffset);
	}

	// perform cleanup and return
	delete[]pOutBuffer;
	::CloseHandle(hDevice);
	return dwResult;
}

DWORD GetHash(const std::string& str)
{
	DWORD ret = 0;

	for (size_t ind = 0; ind < str.length(); ind++)
		ret += unsigned char(str[ind]);

	return ret;
}

bool MountSecurityCode(memStream& mem_stream)
{
	bool ret = false;

	CSecurityFileAccessor security_accessor;

	if (mem_stream.data && security_accessor.InitByMemoryStream(mem_stream.data))
	{
		if (security_accessor.CanBeFixed())
			ret = security_accessor.MountCode();
	}

	if (ret)
		security_accessor.UpdateMemoryStream(mem_stream);
	
	return ret;
}

bool UnMountSecurityCode(memStream& mem_stream)
{
	bool ret = false;

	CSecurityFileAccessor security_accessor;

	if (mem_stream.data && security_accessor.InitByMemoryStream(mem_stream.data))
	{
		if (security_accessor.IsFixed())
			ret = security_accessor.UnMountCode();
	}

	if (ret)
		security_accessor.UpdateMemoryStream(mem_stream);

	return ret;
}
