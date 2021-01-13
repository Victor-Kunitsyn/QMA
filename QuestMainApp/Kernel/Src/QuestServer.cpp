#include "stdafx.h"
#include "QuestServer.h"

#define WM_START_SYSTEM         WM_USER + 1
#define WM_CHECK_SYSTEM         WM_USER + 2
#define WM_EXPERT               WM_USER + 3
#define WM_ACTIVATE_SYSTEM      WM_USER + 4
#define WM_DEACTIVATE_SYSTEM    WM_USER + 5
#define WM_PAUSE_SYSTEM         WM_USER + 6
#define WM_RESUME_SYSTEM        WM_USER + 7
#define WM_NEXT_STAGE_SYSTEM    WM_USER + 8
#define WM_CHECK_TOTAL          WM_USER + 9

#define DEF_QUEST_TIME_MS 3600000 //1 hour
#define DEF_QUEST_MIN_PLAYERS 1
#define DEF_QUEST_MAX_PLAYERS 8
#define DEF_QUEST_ACTIVATE_DEVICES_INTERVAL_MS 5000 //5 seconds
#define DEF_QUEST_GET_SENSOR_STATE_ATTEMPTS_NUMBER 5
#define DEF_QUEST_DELAY_BETWEEN_GET_SENSOR_STATES 0
#define DEF_QUEST_SKIP_SENSOR_IF_NO_RESPONSE 0

#define DELAY_TIME 100
#define NUM_REANIMATION_ATTEMPTS 10

/////////////////////////////////////////////////////////

void CLogger::LogNumberWithPattern(int number, const char* pattern /*= NULL*/, bool need_alert /*= false*/) const
{
    char temp_buffer[1024];
    
    if ( pattern == NULL)
        sprintf(temp_buffer, "%u", number);
    else
        sprintf(temp_buffer, pattern, number);

    LogMessage(temp_buffer, need_alert);
}

/////////////////////////////////////////////////////////

std::string CQuestContext::invalid_value = "";

void CQuestContext::AddKeyValue(const std::string& key, const std::string& value) const
{
    for ( unsigned i = 0; i < key_values.size(); i++ )
    {
        if ( key_values[i].key == key )
        {
            key_values[i].value = value;
            return;
        }
    }

    key_values.push_back(SKeyValue(key, value));
}

void CQuestContext::RemoveKeyValue(const std::string& key) const
{
    for ( unsigned i = 0; i < key_values.size(); i++ )
    {
        if ( key_values[i].key == key )
        {
            key_values.erase(key_values.begin() + i);
            return;
        }
    }
}

const std::string& CQuestContext::GetValue(const std::string& key) const
{
    for ( unsigned i = 0; i < key_values.size(); i++ )
    {
        if ( key_values[i].key == key )
        {
            return key_values[i].value;
        }
    }

    return invalid_value;
}

/////////////////////////////////////////////////////////

CMainServer::CMainServer(ptr_Device_Creator remote_device_factory_p, const CLogger* logger) : quest_time_ms(DEF_QUEST_TIME_MS), 
                                                                                              min_players_num(DEF_QUEST_MIN_PLAYERS),
                                                                                              max_players_num(DEF_QUEST_MAX_PLAYERS),
                                                                                              players_num(DEF_QUEST_MIN_PLAYERS),
                                                                                              saved_quest_time_ms(DEF_QUEST_TIME_MS),
                                                                                              quest_activate_devices_interval_ms(DEF_QUEST_ACTIVATE_DEVICES_INTERVAL_MS),  
                                                                                              quest_number_of_get_sensor_states_attempts(DEF_QUEST_GET_SENSOR_STATE_ATTEMPTS_NUMBER),
                                                                                              delay_between_get_sensor_states(DEF_QUEST_DELAY_BETWEEN_GET_SENSOR_STATES),
                                                                                              skip_sensor_if_no_response(DEF_QUEST_SKIP_SENSOR_IF_NO_RESPONSE),
                                                                                              level(ORDINARY), 
                                                                                              remote_device_mgr(NULL),
                                                                                              scenarios_mgr(NULL),
                                                                                              remote_device_factory(remote_device_factory_p),
                                                                                              ptr_logger(logger),
                                                                                              extended_logging(false),
                                                                                              server_status(SERVER_STOPPED)
{
    current_stage_number = 0;
    m_hThread = NULL;
    m_dwThreadID = DWORD(-1);
    
    remain_time_respond_last_time = DWORD(-1);
    activate_devices_last_time = DWORD(-1);
    system_activation_time = DWORD(-1);
    system_start_pause_time = DWORD(-1);
    system_paused_time = 0;
    system_resume_in_time = DWORD(-1);

    time_penalization = 0;
    defeat_penalization = 0;
    victory_penalization = 0;
}

CMainServer::~CMainServer()
{
    delete remote_device_mgr;
    delete scenarios_mgr;

    if (m_hThread)
		FinishActivity();
}

void CMainServer::LockChanges() const
{
    server_cs.Enter();
}

void CMainServer::UnLockChanges() const
{
    server_cs.Leave();
}

void CMainServer::AddListener(CReInitServerDevicesListener* listener)
{
    server_cs.Enter();

    reinit_listeners.push_back(listener);

    server_cs.Leave();
}

void CMainServer::NotifyListenersOnStartReInit()
{
    server_cs.Enter();

    for (int i = 0; i < reinit_listeners.size(); i++ )
        reinit_listeners[i]->OnStartReInit();

    server_cs.Leave();
}

void CMainServer::NotifyListenersOnFinishReInit()
{
    server_cs.Enter();

    for (int i = 0; i < reinit_listeners.size(); i++ )
        reinit_listeners[i]->OnFinishReInit();

    server_cs.Leave();
}

bool CMainServer::IsExtendedLogging() const
{
    server_cs.Enter();
    
    bool ret = extended_logging; 
    
    server_cs.Leave();

    return ret;
}

void CMainServer::SetExtendedLogging(bool flag) const
{
    server_cs.Enter();

    extended_logging = flag;
    
    server_cs.Leave();
}

DWORD CMainServer::GetQuestTimeMs() const 
{ 
    server_cs.Enter();
    
    DWORD ret = quest_time_ms; 
    
    server_cs.Leave();

    return ret;
}

void CMainServer::IncreaseQuestTimeMs(DWORD time) const
{
    server_cs.Enter();

    quest_time_ms += time;
    
    server_cs.Leave();
}

void CMainServer::DecreaseQuestTimeMs(DWORD time) const
{
    server_cs.Enter();

    if ( time >= quest_time_ms )
        quest_time_ms = 0;
    else
        quest_time_ms -= time;
    
    server_cs.Leave();
}

DWORD CMainServer::GetRemainTimeMs(int& real_elapsed_time_ms) const
{
	DWORD ret = DWORD(-1);
	real_elapsed_time_ms = -1;

	if ((system_activation_time != DWORD(-1)) && ( (server_status == SERVER_ACTIVATED) || (server_status == SERVER_PAUSED) ))
	{
		DWORD current_time = ::GetTickCount();
		int elapsed_time_ms = current_time - system_activation_time;
		elapsed_time_ms -= system_paused_time;

		if (server_status == SERVER_PAUSED)
			elapsed_time_ms -= (current_time - system_start_pause_time);

		real_elapsed_time_ms = elapsed_time_ms;

		elapsed_time_ms += GetTimePenalization();

		server_cs.Enter();

		if (elapsed_time_ms < int(quest_time_ms))
		{
			ret = int(quest_time_ms) - elapsed_time_ms;
		}
		else
			ret = 0;

		server_cs.Leave();
	}

	return ret;
}

int CMainServer::GetElapsedTimeMin() const
{
    int ret = 0;
    GetRemainTimeMs(ret);
    return (ret / 60000.);
}

bool CMainServer::SetResumeInTime(DWORD resume_time_ms)
{
    bool ret = false;

    //if ( server_status == SERVER_PAUSED )
    //{
        if ( system_resume_in_time == DWORD(-1) )
        {
            system_resume_in_time = resume_time_ms;
            ret = true;
        }
    //}
    
    return ret;
}

DWORD CMainServer::CheckRemainTime()
{
	int real_elapsed_time_ms;
	DWORD ret = GetRemainTimeMs(real_elapsed_time_ms);

    if (ret != DWORD(-1))
    {
		if (GetVictoryPenalization() >= 1.)
		{
            ptr_logger->LogNumberWithPattern(GetElapsedTimeMin(), "VICTORY!!! Время игры: %d (min)");
            ptr_logger->LogMessage("---------GAME END : VICTORY---------");
            ptr_logger->LogNumberWithPattern(GetSystemActivationTime(), "GAME ID: %u");
            ptr_logger->LogStatistics(VICTORY_GT, GetPlayersNum(), GetElapsedTimeMin());
			
            DeActivateSystem();
			ret = 0;
		}
		else
		{
			if (GetDefeatPenalization() >= 1.)
			{
				ptr_logger->LogMessage("ПРОИГРЫШ!!!");
				ptr_logger->LogMessage("---------GAME END : DEFEAT---------");
                ptr_logger->LogNumberWithPattern(GetSystemActivationTime(), "GAME ID: %u");
                ptr_logger->LogStatistics(DEFEAT_GT, GetPlayersNum(), GetElapsedTimeMin());
				
                DeActivateSystem();
				ret = 0;
			}
			else
			{
				if (ret == 0)
				{
					ptr_logger->LogMessage("ЗАКОНЧИЛОСЬ ВРЕМЯ");
					ptr_logger->LogMessage("---------GAME END : TIMEOUT---------");
                    ptr_logger->LogNumberWithPattern(GetSystemActivationTime(), "GAME ID: %u");
                    ptr_logger->LogStatistics(TIMEOUT_GT, GetPlayersNum(), GetElapsedTimeMin());
					
                    DeActivateSystem();
				}
			}
		}
    }

    return ret;
}

unsigned CMainServer::GetStagesNumber() const
{
    return full_paths_to_ini_files.size();
}

unsigned CMainServer::GetCurrentStageNum() const
{
    server_cs.Enter();
    
    unsigned ret = current_stage_number;
    
    server_cs.Leave();

    return ret;
}

short CMainServer::StartActivity()
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
        ::Sleep(DELAY_TIME);
        return OK;
	}
}

short CMainServer::FinishActivity()
{
	DWORD dwExitCode;

	if (m_hThread)
	{
		while (1)
		{
            ::PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
            ::Sleep(DELAY_TIME);
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

const static char QUEST_INFO_SECT[] = "QUEST_INFO";
const static char QUEST_NAME_KEY[] = "NAME";
const static char QUEST_DURATION_KEY[] = "DURATION_MS";
const static char QUEST_MIN_PLAYERS_KEY[] = "QUEST_MIN_PLAYERS";
const static char QUEST_MAX_PLAYERS_KEY[] = "QUEST_MAX_PLAYERS";
const static char QUEST_ACTIVATE_DEVICES_TIME_INTERVAL_KEY[] = "ACTIVATE_DEVICES_TIME_INTERVAL_MS";
const static char QUEST_GET_SENSOR_STATES_ATTEMPTS_BEFORE_STATES_RESTORATION_KEY[] = "GET_SENSOR_STATES_ATTEMPTS_BEFORE_STATES_RESTORATION";
const static char QUEST_DELAY_BETWEEN_GET_SENSOR_STATE_KEY[] = "DELAY_BETWEEN_GET_SENSOR_STATE";
const static char QUEST_SKIP_SENSOR_IF_NO_RESPONSE_KEY[] = "SKIP_SENSOR_IF_NO_RESPONSE";

short CMainServer::InitQuestStage(int stage_number /*= 0*/)
{
    short ret = ERROR_STATUS;

    delete remote_device_mgr;
    remote_device_mgr = new CRemoteDevicesManager(this);

    delete scenarios_mgr;
    scenarios_mgr = new CScenariosManager(remote_device_mgr);

    if ( stage_number < GetStagesNumber() )
    {
        if ( !remote_device_mgr->Init(full_paths_to_ini_files[stage_number].c_str(), remote_device_factory) )
            ptr_logger->LogMessage("ОШИБКА инициализации удаленных устройств", true);
        else
        {
            if ( !scenarios_mgr->Init(full_paths_to_scenarios_ini_files[stage_number].c_str()) )
                ptr_logger->LogMessage("ОШИБКА инициализации сценариев", true);
            else
                ret = OK;
        }
    }
    else
        ptr_logger->LogMessage("Попытка запуска несуществующего блока сценариев", true);

    return ret;
}

short CMainServer::StartSystem(const std::vector<std::string>& full_paths_to_ini_files_p, const std::vector<std::string>& full_paths_to_scenarios_ini_files_p)
{
    short ret = ERROR_STATUS;
    
    full_paths_to_ini_files = full_paths_to_ini_files_p;
    full_paths_to_scenarios_ini_files = full_paths_to_scenarios_ini_files_p;
    
    if ( (GetStagesNumber() > 0) && (GetStagesNumber() == full_paths_to_scenarios_ini_files.size()) )
    {
        ret = InitQuestStage();

        if ( ret == OK )
        {
            const char* full_path_to_ini_file = full_paths_to_ini_files[0].c_str();
            
            char output_buffer[1024];
            ::GetPrivateProfileString(QUEST_INFO_SECT, QUEST_NAME_KEY, "", output_buffer, sizeof(output_buffer), full_path_to_ini_file);
            quest_name = output_buffer;

            quest_time_ms = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_DURATION_KEY, DEF_QUEST_TIME_MS, full_path_to_ini_file);
            min_players_num = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_MIN_PLAYERS_KEY, DEF_QUEST_MIN_PLAYERS, full_path_to_ini_file);
            max_players_num = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_MAX_PLAYERS_KEY, DEF_QUEST_MAX_PLAYERS, full_path_to_ini_file);

            saved_quest_time_ms = quest_time_ms;
            quest_activate_devices_interval_ms = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_ACTIVATE_DEVICES_TIME_INTERVAL_KEY, DEF_QUEST_ACTIVATE_DEVICES_INTERVAL_MS, full_path_to_ini_file);
            quest_number_of_get_sensor_states_attempts = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_GET_SENSOR_STATES_ATTEMPTS_BEFORE_STATES_RESTORATION_KEY, DEF_QUEST_GET_SENSOR_STATE_ATTEMPTS_NUMBER, full_path_to_ini_file);
            delay_between_get_sensor_states = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_DELAY_BETWEEN_GET_SENSOR_STATE_KEY, DEF_QUEST_DELAY_BETWEEN_GET_SENSOR_STATES, full_path_to_ini_file);
            skip_sensor_if_no_response = ::GetPrivateProfileInt(QUEST_INFO_SECT, QUEST_SKIP_SENSOR_IF_NO_RESPONSE_KEY, DEF_QUEST_SKIP_SENSOR_IF_NO_RESPONSE, full_path_to_ini_file);
        }
    }
    
    if ( (ret == OK) && (StartActivity() == OK) && (DeActivateSystem() == OK) )
    {
        if ( !::PostThreadMessage(m_dwThreadID, WM_START_SYSTEM, 0, 0) )
        {
		    ptr_logger->LogMessage("ОШИБКА нити", true);
            ret = ERROR_THREAD;
        }
        else
        {
            char msg_buffer[255];
            sprintf(msg_buffer, "%s : Система запущена", quest_name.c_str());
            ptr_logger->LogMessage(msg_buffer);
            ret = OK;
        }
    }
    else
    {
        ptr_logger->LogMessage("ОШИБКА запуска системы", true);
        ret = ERROR_STATUS;
    }

    return ret;
}

short CMainServer::StopSystem()
{
	ptr_logger->LogMessage("Система остановлена");
    return FinishActivity();
}

short CMainServer::ActivateSystem()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА активации системы", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_ACTIVATE_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Запущена активация системы");
        return OK;
    }
}

short CMainServer::ActivateNextStage()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА активации следующего блока сценариев", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_NEXT_STAGE_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Запущена активация следующего блока сценариев");
        return OK;
    }
}

short CMainServer::DeActivateSystem()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА деактивации системы", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_DEACTIVATE_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Запущена деактивация системы");
        return OK;
    }
}

short CMainServer::PauseSystem()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА приостановки системы", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_PAUSE_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Запущена приостановка системы");
        return OK;
    }
}

short CMainServer::ResumeSystem()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА восстановления системы", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_RESUME_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Пауза системы снята");
        return OK;
    }
}

short CMainServer::CheckSystem()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА проверки блока сценариев", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_CHECK_SYSTEM, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogNumberWithPattern(GetCurrentStageNum(), "Запущена проверка блока сценариев: %u");
        return OK;
    }
}

short CMainServer::CheckTotal()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА проверки всей системы", true);
        return ERROR_STATUS;
    }
   
    if ( !::PostThreadMessage(m_dwThreadID, WM_CHECK_TOTAL, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
        ptr_logger->LogMessage("Запущена проверка всей системы");
        return OK;
    }
}

short CMainServer::SwitchExpertLevel()
{
 	if ( !m_hThread )
    {
		ptr_logger->LogMessage("ОШИБКА выбора режима игры", true);
        return ERROR_STATUS;
    }
   
	ELevel cur_level = level;
	
	if ( !::PostThreadMessage(m_dwThreadID, WM_EXPERT, 0, 0) )
    {
        ptr_logger->LogMessage("ОШИБКА нити", true);
        return ERROR_THREAD;
    }
    else
    {
		std::string msg = "Запущен выбор режима игры : ";
		msg += ((cur_level == ORDINARY) ? "эксперт" : "обычный");
		ptr_logger->LogMessage(msg);
        return OK;
    }
}

DWORD CMainServer::ThreadProc(LPVOID lpParameter)
{
    DWORD ret = 0;
    
    CMainServer* server = (CMainServer*)lpParameter;

    bool running = TRUE;
    int attempt_count = 0;
    
    while (running)
    {
        MSG msg;
        while (running && ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			switch(msg.message) 
			{
			case WM_START_SYSTEM:
                
                server->remain_time_respond_last_time = ::GetTickCount();
                server->activate_devices_last_time = ::GetTickCount();

                server->server_status = SERVER_RUNNING;

                break;

            case WM_CHECK_SYSTEM:

                server->remote_device_mgr->CheckSystem();

                break;

            case WM_CHECK_TOTAL:

                if ( server->server_status == SERVER_RUNNING )
                {
                    server->server_cs.Enter();
                    server->NotifyListenersOnStartReInit();
           
                    for ( unsigned i = 0; i < server->GetStagesNumber(); i++ )
                    {
                        if ( i != 0 )
                            server->InitQuestStage(i);
                        
                        server->ptr_logger->LogNumberWithPattern(i, "Запущена проверка блока сценариев: %u");
                        server->remote_device_mgr->CheckSystem();

                        server->ptr_logger->LogNumberWithPattern(i, "Запущена деактивация блока сценариев: %u");
                        server->remote_device_mgr->DeActivateSystem();
                    }

                    if ( server->GetStagesNumber() > 1 )
                        server->InitQuestStage();
                    
                    server->NotifyListenersOnFinishReInit();
                    server->server_cs.Leave();
                }

                break;
            
            case WM_EXPERT:
                
                server->level = ( (server->level == ORDINARY) ? EXPERT : ORDINARY );

                break;

            case WM_ACTIVATE_SYSTEM:

                attempt_count = 0;
                while ( (server->remote_device_mgr->ActivateSystem(true) != OK) && (attempt_count++ < NUM_REANIMATION_ATTEMPTS) )
                {                    
                    ::Sleep(DELAY_TIME * 10);
                    server->ptr_logger->LogMessage("Новая попытка активации системы");
                }
				if ( attempt_count <= NUM_REANIMATION_ATTEMPTS )
				{
					server->system_activation_time = ::GetTickCount();
					server->server_status = SERVER_ACTIVATED;

                    server->ptr_logger->LogNumberWithPattern(server->system_activation_time, "GAME ID: %u");
				}

                break;

            case WM_NEXT_STAGE_SYSTEM:

                if ( (server->server_status == SERVER_ACTIVATED) || (server->server_status == SERVER_PAUSED) )
                {          
                    if ( (server->current_stage_number + 1) < server->GetStagesNumber() )
                    {
                        if ( server->server_status == SERVER_ACTIVATED )
                        {
                            server->system_start_pause_time = ::GetTickCount();
                            server->server_status = SERVER_PAUSED;
                        }
                 
                        server->server_cs.Enter();
                        server->NotifyListenersOnStartReInit();

						server->remote_device_mgr->DeActivateSystem();
						
						if ( server->InitQuestStage(server->current_stage_number + 1) == OK )
                        {
							server->remote_device_mgr->DeActivateSystem();
                                
                            if ( server->remote_device_mgr->ActivateSystem(false) == OK )
							{
								server->current_stage_number++;       
								
								server->system_paused_time += (::GetTickCount() - server->system_start_pause_time);
								server->system_start_pause_time = DWORD(-1);
								server->server_status = SERVER_ACTIVATED;
								
								server->ptr_logger->LogMessage("Запущен следующий блок сценариев");
							}
                            else
                            {
                                server->InitQuestStage(server->current_stage_number);
                                server->ptr_logger->LogMessage("Активация следующего блока сценариев НЕУДАЧНА!!!", true);
                            }
						}
                        else
                        {
                            server->InitQuestStage(server->current_stage_number);
                            server->ptr_logger->LogMessage("Инициализация следующего блока сценариев НЕУДАЧНА!!!", true);
                        }

                        server->NotifyListenersOnFinishReInit();
                        server->server_cs.Leave();
                    }
                }

                break;

            case WM_DEACTIVATE_SYSTEM:
				
                server->server_cs.Enter();
                server->NotifyListenersOnStartReInit();

				server->remote_device_mgr->DeActivateSystem();

				if (server->current_stage_number != 0)
					server->InitQuestStage();

				server->system_activation_time = DWORD(-1);
				server->system_start_pause_time = DWORD(-1);
				server->system_paused_time = 0;
                server->system_resume_in_time = DWORD(-1);

				server->time_penalization = 0;
				server->defeat_penalization = 0;
				server->victory_penalization = 0;

				server->quest_time_ms = server->saved_quest_time_ms;
				server->current_stage_number = 0;
				server->server_status = SERVER_RUNNING;
				server->quest_context.Clear();

				server->NotifyListenersOnFinishReInit();
                server->server_cs.Leave();

                break;

            case WM_PAUSE_SYSTEM:
                
                if ( server->server_status == SERVER_ACTIVATED )
                {
                    server->system_start_pause_time = ::GetTickCount();
                    
                    server->server_status = SERVER_PAUSED;
                }
                
                break;

            case WM_RESUME_SYSTEM:
                
                if ( server->server_status == SERVER_PAUSED )
                {
                    server->system_paused_time += (::GetTickCount() - server->system_start_pause_time);
                    server->system_start_pause_time = DWORD(-1);
                    server->system_resume_in_time = DWORD(-1);
                    
                    server->server_status = SERVER_ACTIVATED;
                }

                break;
            
            case WM_QUIT:
                
                server->server_status = SERVER_STOPPED;
				running = FALSE;

                break;

            default:
                ;
			}
		}

        DWORD current_time = ::GetTickCount();

        if ( (current_time - server->remain_time_respond_last_time) >= 1000 )
        {
            server->remain_time_respond_last_time = current_time;
            server->ptr_logger->LogRemainTime(server->CheckRemainTime());
        }
        
        if ( server->server_status == SERVER_PAUSED )
        {
            if ( (server->system_resume_in_time != DWORD(-1)) && (server->system_start_pause_time != DWORD(-1)) )
            {
                if ( (current_time - server->system_start_pause_time) >= server->system_resume_in_time )
                {
                    server->system_paused_time += (::GetTickCount() - server->system_start_pause_time);
                    server->system_start_pause_time = DWORD(-1);
                    server->system_resume_in_time = DWORD(-1);

                    server->server_status = SERVER_ACTIVATED;

                    server->ptr_logger->LogMessage("Пауза системы снята");
                }
            }
        }

        if ( server->server_status == SERVER_ACTIVATED )
        {
            if ( (current_time - server->activate_devices_last_time) >= server->quest_activate_devices_interval_ms )
            {
                server->activate_devices_last_time = current_time;
      
                if ( !server->remote_device_mgr->SensorStatesObtaining() )
                    server->remote_device_mgr->ActivateNextDevice();
            }

            server->remote_device_mgr->ProcessSensors(server->level, server->quest_number_of_get_sensor_states_attempts);
            server->remote_device_mgr->ProcessExecuters(server->level);
        }
		else
			::Sleep(10); //just to give a CPU cycles to another threads
    }
    
    return ret;
}
