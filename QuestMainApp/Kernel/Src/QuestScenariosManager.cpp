#include "stdafx.h"
#include <strstream>

#include "QuestServer.h"
#include "QuestScenariosManager.h"
#include "QuestRemoteDevicesManager.h"
#include "QuestRemoteExecuter.h"

CScenariosManager::CScenariosManager(const CRemoteDevicesManager* ptr_device_mgr_p) : ptr_device_mgr(ptr_device_mgr_p)
{
}

CScenariosManager::~CScenariosManager()
{
}

const static char SCENARIO_SECT[] = "SCENARIOS";
const static char SCENARIO_NUM_KEY[] = "NUM";
const static char SCENARIO_SECT_PATTERN[] = "SCENARIO_%d";
const static char SCENARIO_NAME_KEY[] = "NAME";
const static char SCENARIO_ACTIONS_KEY[] = "ACTIONS";
const static char SCENARIO_TEXT_COLOR_KEY[] = "TEXT_COLOR";
const static char SCENARIO_BK_COLOR_KEY[] = "BK_COLOR";

const static char SENSOR_STATE_DEACTIVATED[] = "DEACTIVATED";
const static char SENSOR_STATE_ACTIVATED[] = "ACTIVATED";
const static char SENSOR_STATE_ALARMED[] = "ALARMED";
const static char SENSOR_STATE_DISARMED[] = "DISARMED";
const static char SENSOR_STATE_SLEEP[] = "SLEEP";

struct SStateStrToState
{
    SStateStrToState(const char* str, ESensorState s_s) : sensor_state_str(str), sensor_state(s_s) {}
    
    const char* sensor_state_str;
    ESensorState sensor_state;
};

const static SStateStrToState state_str_to_state[LAST_STATE] = 
{
    SStateStrToState( SENSOR_STATE_DEACTIVATED, DEACTIVATED ),
    SStateStrToState( SENSOR_STATE_ACTIVATED, ACTIVATED ),
    SStateStrToState( SENSOR_STATE_ALARMED, ALARMED ),
    SStateStrToState( SENSOR_STATE_DISARMED, DISARMED ),
    SStateStrToState( SENSOR_STATE_SLEEP, SLEEP )
};

ESensorState GetSensorState(const char* sensor_state)
{
    ESensorState ret = LAST_STATE;

    for ( int i = 0; i < LAST_STATE; i++ )
    {
        if ( strcmp(sensor_state, state_str_to_state[i].sensor_state_str) == 0 ) 
        {
            ret = state_str_to_state[i].sensor_state;
            break;
        }
    }
    
    return ret;
}

std::string GetSensorStateStr(ESensorState sensor_state, bool add_sensor_value)
{
    std::string ret = "";
    
    ESensorStateEnum sensor_state_norm = sensor_state.GetNormalized();

    if ( sensor_state_norm < LAST_STATE )
    {
        ret = state_str_to_state[sensor_state_norm].sensor_state_str;
        
        if ( add_sensor_value )
        {
            ret += ", ";
            
            char tmp_buffer[12];
            sprintf(tmp_buffer, "%u", sensor_state.GetValue());
            ret += tmp_buffer;
        }
    }

    return ret;
}

bool CScenariosManager::Init(const char* scenario_ini_file_full_path)
{
    bool ret = (ptr_device_mgr != NULL);

    scenarios.clear();

    if ( ret && (scenario_ini_file_full_path != NULL) )
    {
        int num_scenarios = ::GetPrivateProfileInt(SCENARIO_SECT, SCENARIO_NUM_KEY, 0, scenario_ini_file_full_path);

        if ( num_scenarios > 0 )
        {
            char current_sect[30];
            char output_buffer[1024];

            for ( int i = 0; (i < num_scenarios) && ret; i++ )
            {
                SScenario cur_scenario;
                
                sprintf(current_sect, SCENARIO_SECT_PATTERN, i);

                ::GetPrivateProfileString(current_sect, SCENARIO_NAME_KEY, "", output_buffer, sizeof(output_buffer), scenario_ini_file_full_path);
                cur_scenario.name = output_buffer;

                cur_scenario.color = ::GetPrivateProfileInt(current_sect, SCENARIO_TEXT_COLOR_KEY, 0, scenario_ini_file_full_path);
                cur_scenario.bk_color = ::GetPrivateProfileInt(current_sect, SCENARIO_BK_COLOR_KEY, DEF_BK_COLOR, scenario_ini_file_full_path);

				::GetPrivateProfileString(current_sect, SCENARIO_ACTIONS_KEY, "", output_buffer, sizeof(output_buffer), scenario_ini_file_full_path);

                std::istrstream actions_stream(output_buffer);
                int actions_num;

                actions_stream >> actions_num;

                if ( actions_num > 0 )
                {
                    char action_buffer[1024];

                    for ( int j = 0; (j < actions_num) && ret; j++ )
                    {
                        SScenarioAction cur_scenarion_action;
                        
                        int device_ordinal;
                        actions_stream >> device_ordinal;

                        if ( (device_ordinal >= 0) && (device_ordinal < ptr_device_mgr->GetDevices().size()) )
                        {
                            action_buffer[0] = 0x0;
                            actions_stream >> action_buffer;

                            ESensorState action_sensor_state = GetSensorState(action_buffer);

                            if ( action_sensor_state != LAST_STATE ) //action for sensor
                            {
                                if ( ptr_device_mgr->GetDevices()[device_ordinal]->GetSensor() != NULL )
                                {
                                    cur_scenarion_action.device_ord = device_ordinal;
                                    cur_scenarion_action.sensor_state = action_sensor_state;
                                    cur_scenarion_action.executer_param = "";
                                    cur_scenarion_action.is_execute_action = false;

                                    cur_scenario.actions.push_back(cur_scenarion_action);
                                }
                                else
                                    ret = false;
                            }
                            else //action for executer
                            {
								if (action_buffer[0] == '!')
								{
									cur_scenarion_action.device_ord = device_ordinal;
									cur_scenarion_action.sensor_state = LAST_STATE;

									cur_scenarion_action.executer_param = action_buffer + 1;

									int exec_param_len = cur_scenarion_action.executer_param.length();

									if ((exec_param_len == 0) || (cur_scenarion_action.executer_param[exec_param_len - 1] != '!'))
									{
										action_buffer[0] = 0x0;
										actions_stream.getline(action_buffer, sizeof(action_buffer), '!');
										cur_scenarion_action.executer_param += action_buffer;
									}
									else
										cur_scenarion_action.executer_param[exec_param_len - 1] = 0x0;

									cur_scenarion_action.is_execute_action = true;

									cur_scenario.actions.push_back(cur_scenarion_action);
								}
								else
									ret = false;
                            }
                        }
                        else
                            ret = false;
                    }

                    if ( ret )
                        scenarios.push_back(cur_scenario);
                }
                else
                    ret = false;
            }
        }
    }

    if ( !ret )
        scenarios.clear();

    return ret;
}

bool CScenariosManager::ExecuteScenario(int scenario_ind) const
{
    bool ret = false;

    if ( (ptr_device_mgr != NULL) && (scenario_ind < GetScenariosNumber()) )
    {
        ret = true;

        char msg_buffer[1024];

        const SScenario* scenario = GetScenario(scenario_ind);
        
        for ( int i = 0; (i < scenario->actions.size()) && ret; i++ )
        {
            const SScenarioAction& scenario_action = scenario->actions[i];
            const CRemoteDevice* device = ptr_device_mgr->GetDevices()[scenario_action.device_ord];

            if ( device != NULL )
            {
				if (scenario_action.is_execute_action) //should perform execute action
				{
					ERemoteDeviceResponse executed = device->DoAction(ptr_device_mgr, scenario_action.executer_param.c_str());

					ret = ret && (executed == TRUE_RESPONSE);

					if (!ret && ptr_device_mgr->GetLogger())
					{
						if (executed == FALSE_RESPONSE)
						{
							sprintf(msg_buffer, "Â ÐÅÆÈÌÅ ÓÏÐÀÂËÅÍÈß ÑÖÅÍÀÐÈßÌÈ ÍÅ ñðàáîòàëî èñï. óñòð.: %s, ñ ïàð.: %s", device->GetName().c_str(), scenario_action.executer_param.c_str());
							ptr_device_mgr->GetLogger()->LogMessage(msg_buffer, true);
						}
						else
						{
							sprintf(msg_buffer, "Â ÐÅÆÈÌÅ ÓÏÐÀÂËÅÍÈß ÑÖÅÍÀÐÈßÌÈ ÍÅÒ îòâåòà îò óñòðîéñòâà: %s", device->GetName().c_str());
							ptr_device_mgr->GetLogger()->LogMessage(msg_buffer, true);
						}
					}
				}
				else //should perform set sensor state
				{
                    if ( device->GetSensor() )
                    {
                        ERemoteDeviceResponse state_set = device->GetSensor()->SetSensorState(scenario_action.sensor_state);

                        ret = ret && (state_set == TRUE_RESPONSE);

                        if ( !ret && ptr_device_mgr->GetLogger() )
                        {
                            if ( state_set == FALSE_RESPONSE )
                            {
                                sprintf(msg_buffer, "Â ÐÅÆÈÌÅ ÓÏÐÀÂËÅÍÈß ÑÖÅÍÀÐÈßÌÈ ÍÅ óñòàíîâëåíî ñîñòîÿíèå äëÿ óñòðîéñòâà: %s", device->GetName().c_str());
                                ptr_device_mgr->GetLogger()->LogMessage(msg_buffer, true);
                            }
                            else
                            {
                                sprintf(msg_buffer, "Â ÐÅÆÈÌÅ ÓÏÐÀÂËÅÍÈß ÑÖÅÍÀÐÈßÌÈ ÍÅÒ îòâåòà îò óñòðîéñòâà: %s", device->GetName().c_str());
                                ptr_device_mgr->GetLogger()->LogMessage(msg_buffer, true);
                            }
                        }
                    }
                    else
                        ret = false;
                }
            }
            else
                ret = false;
        }
    }

    return ret;
}


