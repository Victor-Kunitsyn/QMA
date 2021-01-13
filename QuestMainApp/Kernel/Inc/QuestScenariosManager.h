#pragma once

#include <vector>
#include <string>

#include "QuestRemoteDevice.h"
#include "QuestRemoteSensor.h"

class CRemoteDevicesManager;

struct SScenarioAction
{
    SScenarioAction() : device_ord(-1), sensor_state(LAST_STATE), is_execute_action(false) {}
    
    int device_ord;
    ESensorState sensor_state;
    std::string executer_param;
    bool is_execute_action;
};

#define DEF_BK_COLOR 13160660 //default Windows button background color

struct SScenario
{
    SScenario() : color(0), bk_color(DEF_BK_COLOR) {}

    std::string name;
    std::vector <SScenarioAction> actions;
	
    //members below are used for UI scenario buttons only
    DWORD color;
    DWORD bk_color;
};

class CScenariosManager
{
public:
    CScenariosManager(const CRemoteDevicesManager* ptr_device_mgr_p);
    virtual ~CScenariosManager();

public:
    bool Init(const char* scenario_ini_file_full_path);

    int GetScenariosNumber() const { return scenarios.size(); }
    const SScenario* GetScenario(int scenario_ind) const { return ( (scenario_ind < GetScenariosNumber()) ? &(scenarios[scenario_ind]) : NULL ); }

    bool ExecuteScenario(int scenario_ind) const;

private:
    std::vector<SScenario> scenarios;

    const CRemoteDevicesManager* ptr_device_mgr;
};

extern std::string GetSensorStateStr(ESensorState sensor_state, bool add_sensor_value);
extern ESensorState GetSensorState(const char* sensor_state);

