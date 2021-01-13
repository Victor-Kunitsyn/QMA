#pragma once

#include <string>

#include "QuestRemoteDevicesManager.h"
#include "QuestScenariosManager.h"

enum EEndGameType
{
	VICTORY_GT = 0,
	DEFEAT_GT = 1,
	TIMEOUT_GT = 2,
	EXTERNAL_BREAK_GT = 3,
	LAST_TYPE_GT = 4
};

class CLogger
{
public:
    CLogger() {}
    virtual ~CLogger() {}

public:
    virtual void LogMessage(const std::string& msg, bool need_alert = false) const = 0;
    virtual void LogRemainTime(DWORD time_ms) const = 0;

    virtual void LogStatistics(EEndGameType game_type, unsigned players_num, DWORD game_duration) const {}

    void LogNumberWithPattern(int number, const char* pattern = NULL, bool need_alert = false) const;
};

enum EServerStatus
{
	SERVER_STOPPED = 0,
	SERVER_RUNNING = 1,
	SERVER_ACTIVATED = 2,
    SERVER_PAUSED = 3
};

class CReInitServerDevicesListener
{
public:
    CReInitServerDevicesListener() {}
    virtual ~CReInitServerDevicesListener() {}

public:
    virtual void OnStartReInit() = 0;
    virtual void OnFinishReInit() = 0;
};

class CQuestContext
{
public:
    CQuestContext() {}
    virtual ~CQuestContext() {}

    void Clear() { key_values.clear(); }

    void AddKeyValue(const std::string& key, const std::string& value) const;
    void RemoveKeyValue(const std::string& key) const;
    
    const std::string& GetValue(const std::string& key) const;

private:
    struct SKeyValue
    {
        SKeyValue(const std::string& key_p, const std::string& value_p) : key(key_p), value(value_p) {}

        std::string key;
        std::string value;
    };
    
private:
    mutable std::vector<SKeyValue> key_values;
    static std::string invalid_value;
};

class CMainServer 
{
public:
    CMainServer(ptr_Device_Creator remote_device_factory_p, const CLogger* logger);
    virtual ~CMainServer();
	
    short StartSystem( const std::vector<std::string>& full_paths_to_ini_files_p, 
                       const std::vector<std::string>& full_paths_to_scenarios_ini_files_p );

    short StopSystem();
    
    short ActivateSystem();
    short ActivateNextStage();
    
    short DeActivateSystem();
	
    short PauseSystem();
    short ResumeSystem();

    short CheckSystem();
    short CheckTotal();

    short SwitchExpertLevel();

    void LockChanges() const;   //should be called when access to CRemoteDevicesManager or CScenariosManager is obtained
    void UnLockChanges() const; //should be called when access to CRemoteDevicesManager or CScenariosManager is released

    void AddListener(CReInitServerDevicesListener* listener); //adds listener for devices reinitialization event 

public:
	CRemoteDevicesManager* GetDeviceManager() const { return remote_device_mgr; }
    CScenariosManager* GetScenariosManager() const { return scenarios_mgr; }
    
    const CLogger* GetLogger() const { return ptr_logger; }

    EServerStatus GetStatus() const { return server_status; }
	ELevel GetLevel() const { return level; }
    const char* GetQuestName() const { return quest_name.c_str(); }

    bool IsExtendedLogging() const;
    void SetExtendedLogging(bool flag) const;

    DWORD GetQuestTimeMs() const;
    void IncreaseQuestTimeMs(DWORD time) const;
    void DecreaseQuestTimeMs(DWORD time) const;
    DWORD GetSystemActivationTime() const { return system_activation_time; }
	DWORD GetRemainTimeMs(int& real_elapsed_time_ms) const;
    int GetElapsedTimeMin() const;
    bool SetResumeInTime(DWORD resume_time_ms);

    unsigned GetMinPlayersNum() const { return min_players_num; }
    unsigned GetMaxPlayersNum() const { return max_players_num; }
    unsigned GetPlayersNum() const { return players_num; }
    void SetPlayersNum(unsigned num) { players_num = num; }

    DWORD GetDelayBetweenGetSensorStates() const { return delay_between_get_sensor_states; }

    bool SkipSensorIfNoResponse() const { return skip_sensor_if_no_response; }

    unsigned GetStagesNumber() const;
    unsigned GetCurrentStageNum() const;

    const CQuestContext& GetContext() const { return quest_context; }

public:
    int GetTimePenalization() const { return time_penalization; }
    double GetDefeatPenalization() const { return defeat_penalization; }
    double GetVictoryPenalization() const { return victory_penalization; }

    void IncrementTimePenalization(int inc) const { time_penalization += inc; }
    void IncrementDefeatPenalization(double inc) const { defeat_penalization += inc; }
    void IncrementVictoryPenalization(double inc) const { victory_penalization += inc; }

private:
    short StartActivity();
    short FinishActivity();

    DWORD CheckRemainTime();

    static DWORD WINAPI ThreadProc(LPVOID lpParameter);

    short InitQuestStage(int stage_number = 0);

    void NotifyListenersOnStartReInit();
    void NotifyListenersOnFinishReInit();

private:
    std::vector<std::string> full_paths_to_ini_files;
    std::vector<std::string> full_paths_to_scenarios_ini_files;

    unsigned current_stage_number;
    
    HANDLE m_hThread;
    DWORD m_dwThreadID;

    std::string quest_name;

    mutable DWORD quest_time_ms;
    unsigned min_players_num;
    unsigned max_players_num;
    unsigned players_num;
    DWORD saved_quest_time_ms;
    DWORD quest_activate_devices_interval_ms;
    DWORD quest_number_of_get_sensor_states_attempts;
    DWORD delay_between_get_sensor_states;
    bool skip_sensor_if_no_response;

    ELevel level;
    
    DWORD remain_time_respond_last_time;
    DWORD activate_devices_last_time;
    DWORD system_activation_time;
    DWORD system_start_pause_time;
    DWORD system_paused_time;
    DWORD system_resume_in_time;

    mutable int time_penalization;
    mutable double defeat_penalization;
    mutable double victory_penalization;
    
    CRemoteDevicesManager* remote_device_mgr;
    CScenariosManager* scenarios_mgr;

    ptr_Device_Creator remote_device_factory;
    const CLogger* ptr_logger;
    
    mutable bool extended_logging;

    std::vector<CReInitServerDevicesListener*> reinit_listeners;

	EServerStatus server_status;

    CQuestContext quest_context;

    mutable CScopedCriticalSection server_cs;
};
