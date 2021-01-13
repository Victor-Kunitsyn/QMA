#include "stdafx.h"
#include "TenvisIPCamExecuter.h"

#include "ipcam.h"

#include "CURLDownloaderLight.h"

#define SLEEP_TIME 100
#define TEST_WAV_FILE "TestFile.wav"

#define MIN_TIME_BETWEEN_CAMERA_MOVES_MS 10000
#define CAMERA_MOVE_STEP_TIME_MS 1000

//moving steps starting from center, must return to the center
const CTenvisIPCamExecuter::ECameraMoveControl CTenvisIPCamExecuter::camera_move_steps[] = 
{ 
    LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, 
    RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,    
    RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
    LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT
}; 

CTenvisIPCamExecuter::ECameraMoveControl CTenvisIPCamExecuter::GetStopMovingParameter(ECameraMoveControl move_param)
{
    ECameraMoveControl ret = CENTER;

    switch (move_param)
    {
    case UP:
        ret = STOP_UP;
        break;
    case DOWN:
        ret = STOP_DOWN;
        break;
    case LEFT:
        ret = STOP_LEFT;
        break;
    case RIGHT:
        ret = STOP_RIGHT;
        break;
    default:
        ;
    }

    return ret;
}

bool CTenvisIPCamExecuter::Connect()
{
	is_connected = (camera_handle->Connect(inet_addr(device_ip.c_str()), htons(device_port), user.c_str(), password.c_str()) == OK);

	full_path_to_wav_file = "";
	last_move_time = DWORD(-1);
	move_counter = 0;

	if (is_connected)
	{
		::Sleep(SLEEP_TIME * 10);
		camera_handle->PlayVideo();
		::Sleep(SLEEP_TIME);
		camera_handle->StartTalk();
		::Sleep(SLEEP_TIME * 10);
	}

	return is_connected;
}

bool CTenvisIPCamExecuter::DisConnect()
{
	camera_handle->StopTalk();
	::Sleep(SLEEP_TIME * 10);
	camera_handle->StopVideo();
	::Sleep(SLEEP_TIME * 10);

	is_connected = false;
	return ( camera_handle->Disconnect() == OK );
}

CTenvisIPCamExecuter::CTenvisIPCamExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw, 
										   int id, int request_threshold, const std::string& name) : 
                                           CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), 
										   CRemoteExecuter()
{
	camera_handle = new COpr(0);

	current_state = DEACTIVATED;
    m_hThread = NULL;
    m_dwThreadID = DWORD(-1);

	Connect();
}

short CTenvisIPCamExecuter::StartActivity() const
{
	if (m_hThread)
	{
		DWORD dwExitCode;
        ::GetExitCodeThread(m_hThread, &dwExitCode);
		
        if (dwExitCode == STILL_ACTIVE)
			return ERROR_STATUS;
		
        ::CloseHandle(m_hThread);
		m_hThread = NULL;
        m_dwThreadID = DWORD(-1);
	}

    if (NULL == (m_hThread = ::CreateThread(NULL,
										    0,
										    (LPTHREAD_START_ROUTINE)ThreadProc,
										    (LPVOID)this,
										    FALSE,
										    &m_dwThreadID)) )
	{
		return ERROR_THREAD;
	}
	else
	{
        ::SetThreadPriority(m_hThread, THREAD_PRIORITY_HIGHEST);
        ::Sleep(SLEEP_TIME);
        return OK;
	}
}

short CTenvisIPCamExecuter::FinishActivity() const
{
    DWORD dwExitCode;

    if (m_hThread)
	{
		while (1)
		{
            ::PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
            ::Sleep(SLEEP_TIME);
            ::GetExitCodeThread(m_hThread, &dwExitCode);
			if (dwExitCode != STILL_ACTIVE)
				break;
		}
		
        ::CloseHandle(m_hThread);
		m_hThread = NULL;
        m_dwThreadID = DWORD(-1);
		return OK;
	}
	else
	{
		return ERROR_STATUS;
	}
}

static void WriteXMLtoString(void* p_string, const char* buffer, int len)
{
	std::string& str = *((std::string*)p_string);
	str.append(buffer, len);
}

bool CTenvisIPCamExecuter::RebootCamera()
{
	bool ret = false;
	
	CURLDownloaderLight downloader;
	downloader.UseProxy("", "");

	std::string device_reboot_addr = "http://";

	device_reboot_addr += user;
	device_reboot_addr += ":";
	device_reboot_addr += password;
	device_reboot_addr += "@";

	device_reboot_addr += device_ip;
	device_reboot_addr += ":";
	char temp_buffer[10];
	sprintf(temp_buffer, "%d", device_port);
	device_reboot_addr += temp_buffer;
	device_reboot_addr += "/reboot.cgi?user=";
	device_reboot_addr += user;
	device_reboot_addr += "&pwd=";
	device_reboot_addr += password;

	downloader.SetURL(device_reboot_addr.c_str());
	std::string o_result_xml;
	
	downloader.GetData(WriteXMLtoString, &o_result_xml, connection_timeout_ms, data_timeout_ms);

	::Sleep(SLEEP_TIME * 600); //waiting 60 seconds
	
	ret = DisConnect();
	ret = ( ret && Connect() );

	return ret;
}

CTenvisIPCamExecuter::~CTenvisIPCamExecuter()
{
	DisConnect();

	FinishActivity();

	delete camera_handle;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::Check_internal() const
{
    ERemoteDeviceResponse ret = ( (is_connected && camera_handle->bConnected)? TRUE_RESPONSE : FALSE_RESPONSE );
    ret = ret && Activate();
    ret = ret && CRemoteExecuter::DoAction(TEST_WAV_FILE);
    return ret;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::Activate_internal() const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( current_state == DEACTIVATED)
    {
		camera_handle->DecoderControl(CENTER);

		if ( StartActivity() == OK )
        {
            camera_handle->SetCallingThreadId(m_dwThreadID);
            current_state = ACTIVATED;
            ret = TRUE_RESPONSE;
        }
    }
    else
        ret = TRUE_RESPONSE;

    return ret;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::DeActivate_internal() const
{
    FinishActivity();
    current_state = DEACTIVATED;
    
    full_path_to_wav_file = "";
    last_move_time = DWORD(-1);
    move_counter = 0;
    
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::IsActivated_internal() const
{
    return ( (is_connected && (current_state != DEACTIVATED)) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

struct WAVHEADER
{
    char chunkId[4];
    unsigned long chunkSize;
    char format[4];
    char subchunk1Id[4];
    unsigned long subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned long sampleRate;
    unsigned long byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    char subchunk2Id[4];
    unsigned long subchunk2Size;
};

DWORD WINAPI CTenvisIPCamExecuter::ThreadProc(LPVOID lpParameter)
{
    DWORD ret = OK;
    
    CTenvisIPCamExecuter* ip_cam = (CTenvisIPCamExecuter*)lpParameter;

    if ( ip_cam && ip_cam->is_connected )
    {
        BOOL msg_ret;
        MSG msg;

        while ( (msg_ret = ::GetMessage(&msg, NULL, 0, 0)) != 0  )
        {
			if ( (msg.message == WM_REBOOT_CAMERA) || 
				 (((msg.message == WM_STARTTALK_RESULT) || (msg.message == WM_TALK_STOPPED)) && (msg.wParam != OK)) )
			{
				ip_cam->RebootCamera();
				continue;
			}
			
			if ( !ip_cam->camera_handle->bConnected )
				continue;

			if ( (msg.message == WM_ALARM2) && (ip_cam->current_state != DEACTIVATED) )
            {
                short sAlarm = msg.wParam;

                if ( (sAlarm == 1) || (sAlarm == 2) )
                    ip_cam->current_state = ALARMED;
                else
                    ip_cam->current_state = ACTIVATED;

                continue;
            }

            if ( msg.message == WM_VIDEO )
            {
                IMAGE* pImage = (IMAGE*)msg.wParam;

                delete [] pImage->pData;
                delete pImage;

                continue;
            }

            if ( (msg.message == WM_MOVE_CAMERA) && (ip_cam->current_state != DEACTIVATED) )
            {
                DWORD current_time = ::GetTickCount();

                if ( (ip_cam->last_move_time == DWORD(-1)) || ((current_time - ip_cam->last_move_time) >= MIN_TIME_BETWEEN_CAMERA_MOVES_MS) )
                {
                    ECameraMoveControl cur_move = camera_move_steps[ip_cam->move_counter++];

                    ip_cam->camera_handle->DecoderControl(cur_move);
                    ::Sleep(CAMERA_MOVE_STEP_TIME_MS);
                    ip_cam->camera_handle->DecoderControl(GetStopMovingParameter(cur_move));

                    if ( ip_cam->move_counter >= sizeof(camera_move_steps) / sizeof(ECameraMoveControl) )
                        ip_cam->move_counter = 0;

                    ip_cam->last_move_time = current_time;
                }

                continue;
            }

            if ( (msg.message == WM_PLAY_AUDIO_FILE) && (ip_cam->current_state != DEACTIVATED) )
            {
                ip_cam->camera_cs.Enter();

                if ( ip_cam->full_path_to_wav_file != "" )
                {
                    FILE* file;

                    fopen_s(&file, ip_cam->full_path_to_wav_file.c_str(), "rb");

                    if ( file != NULL )
                    {
                        int decode_index = 0;
                        int decode_sample = 0;

                        fseek(file, 0, SEEK_END);
                        fpos_t file_len = 0;
                        fgetpos(file, &file_len);

                        unsigned file_pos = 0;

                        fseek(file, 0, SEEK_SET);
                        WAVHEADER header;
                        fread(&header, sizeof(WAVHEADER), 1, file);

                        file_pos += sizeof(WAVHEADER);

                        int i = 0;
                        while ( file_pos < file_len )
                        {
                            fseek(file, file_pos, SEEK_SET);

                            int raw_data_len = int( (1024 <= (file_len - file_pos)) ? 1024 : (file_len - file_pos) );
                            int raw_data_len_div_4 = raw_data_len / 4;
                            raw_data_len = raw_data_len_div_4 * 4;
                            if ( raw_data_len == 0 )
                                break;

                            unsigned char* raw_data = new unsigned char[raw_data_len];

                            AUDIO* Audio = new AUDIO;

                            Audio->uiDataLen = raw_data_len / 4;
                            Audio->pData = new unsigned char[Audio->uiDataLen];

                            fread(raw_data, 1, raw_data_len, file);
                            adpcm_encode(raw_data, raw_data_len, Audio->pData, &decode_index, &decode_sample);

                            delete[] raw_data;

                            file_pos += raw_data_len;

                            Audio->uiTick = GetTickCount();
                            Audio->uiSeq = i;
                            Audio->iTime = 0;
                            Audio->ucFormat = 0;

                            ip_cam->camera_handle->AddTalkAudio(Audio);

                            #define adjust_time 10
                            int pause_between_packets = int(double(raw_data_len) / header.byteRate * 1000. - adjust_time);
                            if (pause_between_packets < 0)
                                pause_between_packets = 0;

                            ::Sleep(pause_between_packets);

                            i++;
                        }

                        fclose(file);
                    }

                    ip_cam->full_path_to_wav_file = "";
                }

                ip_cam->camera_cs.Leave();

                continue;
            }
        }
    }

    return ret;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::DoAction_internal(const void* param /*= NULL*/) const
{
	ERemoteDeviceResponse ret = FALSE_RESPONSE;

	if ( IsActivated() == TRUE_RESPONSE )
	{
        const char* ptr_pararm_str = (const char*)param;

        if ( (ptr_pararm_str != NULL) && (ptr_pararm_str[0] != 0x0) ) //playing file or camera reboot
        {
			if (strcmp(ptr_pararm_str, "REBOOT") == 0)
			{
				::PostThreadMessage(m_dwThreadID, WM_REBOOT_CAMERA, 0, 0);
				ret = TRUE_RESPONSE;
			}
			else
			{
				std::string path_to_quest_ini = "";
				char exe_file_path[1024];
				int num_cpy = ::GetModuleFileName(NULL, exe_file_path, sizeof(exe_file_path));

				if (num_cpy)
				{
					path_to_quest_ini = exe_file_path;
					path_to_quest_ini.erase(path_to_quest_ini.find_last_of("\\") + 1);
				}

				if ((path_to_quest_ini != "") && is_connected)
				{
					path_to_quest_ini += ptr_pararm_str;

					bool can_play = camera_cs.TryEnter();

					if (can_play)
					{
						full_path_to_wav_file = path_to_quest_ini;

						::PostThreadMessage(m_dwThreadID, WM_PLAY_AUDIO_FILE, 0, 0);

						camera_cs.Leave();
					}

					ret = TRUE_RESPONSE;
				}
			}
        }
        else //moving camera
        {
            ::PostThreadMessage(m_dwThreadID, WM_MOVE_CAMERA, 0, 0);
            ret = TRUE_RESPONSE;
        }
	}

	return ret;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::GetSensorState_internal(ESensorState& state) const
{
    state = current_state;
    return TRUE_RESPONSE;
}

ERemoteDeviceResponse CTenvisIPCamExecuter::SetSensorState_internal(ESensorState state)
{
    ERemoteDeviceResponse ret = TRUE_RESPONSE;
    
    if ( state != DEACTIVATED )
        ret = Activate();

    current_state = state;

    return ret;
}
