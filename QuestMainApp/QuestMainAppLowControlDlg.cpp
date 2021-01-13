#include "stdafx.h"

#include "QuestMainAppLowControlDlg.h"
#include "QuestMainAppDlg.h"

#include "QuestRemoteExecuter.h"

CQuestMainAppLowControlDlg::CQuestMainAppLowControlDlg(const CQuestMainAppDlg* main_app_dlg_p): CPropertyPage(CQuestMainAppLowControlDlg::IDD), 
                                                                                                main_app_dlg(main_app_dlg_p), 
                                                                                                controls_refresh_timer(0),
                                                                                                combos_initialized(false)
{
}

CQuestMainAppLowControlDlg::~CQuestMainAppLowControlDlg()
{
}

void CQuestMainAppLowControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EXECUTER_COMBO, executers_combo);
    DDX_Control(pDX, IDC_EXECUTER_PARAMETER, executer_parameter_ed);
    DDX_Control(pDX, IDC_SENSOR_COMBO, sensors_combo);
    DDX_Control(pDX, IDC_SENSOR_STATE_EDIT, sensor_state_ed);
    DDX_Control(pDX, IDC_SENSOR_STATE_COMBO, sensor_states_combo);
    DDX_Control(pDX, IDC_SENSOR_VALUE_EDIT, sensor_value_ed);
    DDX_Control(pDX, IDC_INC_TIME_EDIT, time_increase_ed);
    DDX_Control(pDX, IDC_DEC_TIME_EDIT, time_decrease_ed);
}

BEGIN_MESSAGE_MAP(CQuestMainAppLowControlDlg, CPropertyPage)
    ON_WM_TIMER()
	ON_BN_CLICKED(IDC_EXECUTE_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedExecuteButton)
    ON_BN_CLICKED(IDC_DISABLE_EXECUTER_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedDisableExecuterButton)
    ON_BN_CLICKED(IDC_ENABLE_EXECUTER_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedEnableExecuterButton)
    ON_BN_CLICKED(IDC_SET_STATE_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedSetStateButton)
    ON_BN_CLICKED(IDC_EMULATION_ON_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedSetEmulationOnButton)
    ON_BN_CLICKED(IDC_EMULATION_OFF_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedSetEmulationOffButton)
    ON_BN_CLICKED(IDC_SET_TIME_INC_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedIncTimeButton)
	ON_BN_CLICKED(IDC_SET_TIME_DEC_BUTTON, &CQuestMainAppLowControlDlg::OnBnClickedDecTimeButton)
END_MESSAGE_MAP()

BOOL CQuestMainAppLowControlDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	EnableDisableControls();

	controls_refresh_timer = SetTimer(1, 500, 0);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQuestMainAppLowControlDlg::OnCancel()
{	
    KillTimer(controls_refresh_timer);
    CPropertyPage::OnCancel();
}

BOOL CQuestMainAppLowControlDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;                // Do not process further
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CQuestMainAppLowControlDlg::OnTimer(UINT nIDEvent)
{
	EnableDisableControls();
}

void CQuestMainAppLowControlDlg::EnableDisableControls()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

        InitCombos();
        
        EServerStatus server_status = main_app_dlg->GetMainServer()->GetStatus();
		
		::EnableWindow(*GetDlgItem(IDC_EXECUTER_COMBO), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_EXECUTER_PARAMETER), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SENSOR_COMBO), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SENSOR_STATE_EDIT), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SENSOR_STATE_COMBO), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_EXECUTE_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_DISABLE_EXECUTER_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_ENABLE_EXECUTER_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_EXECUTER_ON_OFF_STATE), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SET_STATE_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
        ::EnableWindow(*GetDlgItem(IDC_EMULATION_ON_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
        ::EnableWindow(*GetDlgItem(IDC_EMULATION_OFF_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SENSOR_ON_OFF_STATE), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_INC_TIME_EDIT), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_DEC_TIME_EDIT), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SET_TIME_INC_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
		::EnableWindow(*GetDlgItem(IDC_SET_TIME_DEC_BUTTON), (server_status == SERVER_STOPPED ? FALSE : TRUE));
        
        if ( server_status != SERVER_STOPPED )
		{
			int cur_sel = sensors_combo.GetCurSel();
			
			if ( cur_sel != CB_ERR )
			{
				CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

				if ( device_mgr )
				{
					int sensor_ord = sensors_ordinals[cur_sel];

					ESensorState cur_sensor_state = device_mgr->GetSensorStates()[sensor_ord];
					
					CString prev_sensor_state_str; 
					sensor_state_ed.GetWindowText(prev_sensor_state_str);

					if ( prev_sensor_state_str != GetSensorStateStr(cur_sensor_state, true).c_str() )
						sensor_state_ed.SetWindowText(GetSensorStateStr(cur_sensor_state, true).c_str());

					bool sensor_off = device_mgr->GetDevices()[sensor_ord]->GetSensor()->IsStateForced();
					GetDlgItem(IDC_SENSOR_ON_OFF_STATE)->SetWindowText(sensor_off ? "Эмуляция" : "");
				}
			}

			cur_sel = executers_combo.GetCurSel();
		
			if (cur_sel != CB_ERR)
			{
				CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

				if (device_mgr)
				{
					int executer_ord = executers_ordinals[cur_sel];

					bool executer_off = device_mgr->GetDevices()[executer_ord]->GetExecuter()->IsDeviceDisregardered();
					GetDlgItem(IDC_EXECUTER_ON_OFF_STATE)->SetWindowText(executer_off ? "Выключено" : "");
				}
			}		
		}

  		main_app_dlg->GetMainServer()->UnLockChanges();
	}
}

void CQuestMainAppLowControlDlg::InitCombos()
{
    if ( !combos_initialized && main_app_dlg->GetMainServer() && (main_app_dlg->GetMainServer()->GetStatus() != SERVER_STOPPED) )
    {
        main_app_dlg->GetMainServer()->LockChanges();

        executers_combo.ResetContent();
        sensors_combo.ResetContent();
        sensor_states_combo.ResetContent();

        executers_ordinals.clear();
        sensors_ordinals.clear();

        CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

        if (device_mgr)
        {
            const std::vector<CRemoteDevice*>& devices = device_mgr->GetDevices();

            executers_combo.EnableWindow(TRUE);
            sensors_combo.EnableWindow(TRUE);
            sensor_states_combo.EnableWindow(TRUE);

            for ( int i = 0; i < devices.size(); i++ )
            {
                if ( devices[i]->GetExecuter() )
                {
                    executers_combo.AddString(devices[i]->GetName().c_str());
                    executers_ordinals.push_back(i);
                }

                if ( devices[i]->GetSensor() )
                {
                    sensors_combo.AddString(devices[i]->GetName().c_str());
                    sensors_ordinals.push_back(i);
                }
            }

            for ( int i = 0; i < LAST_STATE; i++ )
            {
                sensor_states_combo.AddString(GetSensorStateStr(ESensorState((ESensorStateEnum)i), false).c_str());
            }

            executers_combo.SetCurSel(0);
            sensors_combo.SetCurSel(0);
            sensor_states_combo.SetCurSel(0);

            combos_initialized = true;
        }

        main_app_dlg->GetMainServer()->UnLockChanges();
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedExecuteButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

        CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		ERemoteDeviceResponse executed = FALSE_RESPONSE;

		if ( device_mgr )
		{
			int cur_sel = executers_combo.GetCurSel();
			
			if ( cur_sel != CB_ERR )
			{
				int executer_ord = executers_ordinals[cur_sel];
				
				CString executer_parameter;
				executer_parameter_ed.GetWindowText(executer_parameter);

				executed = device_mgr->GetDevices()[executer_ord]->DoAction(device_mgr, executer_parameter);

				if ( executed == TRUE_RESPONSE )
                {
					std::string msg = "В РУЧНОМ РЕЖИМЕ сработало исп. устр.: " + device_mgr->GetDevices()[executer_ord]->GetName();
                    msg += ", с пар.: " + executer_parameter;
                    
                    bool in_emulated_state = device_mgr->GetDevices()[executer_ord]->IsExecuterInEmulatedState();

                    if ( in_emulated_state )
                        msg += " (режим эмуляции!!!)";

                    main_app_dlg->LogMessage(msg, in_emulated_state);
                }
			}
		}

        main_app_dlg->GetMainServer()->UnLockChanges();

		if (executed == TRUE_RESPONSE)
			MessageBox("Выполнено", "OK");
		else
		{
			if (executed == FALSE_RESPONSE)
				MessageBox("Ошибка выполнения!!!", "Ошибка");
			else
				MessageBox("Нет ответа от устройства!!!", "Ошибка");
		}
	}
}

void CQuestMainAppLowControlDlg::OnBnClickedDisableExecuterButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

		bool ok = false;

		CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		if (device_mgr)
		{
			int cur_sel = executers_combo.GetCurSel();
			
			if ( cur_sel != CB_ERR )
			{
				int executer_ord = executers_ordinals[cur_sel];
                device_mgr->GetDevices()[executer_ord]->GetExecuter()->DisregardDevice();
                
				ok = true;				
            }
        }
        
        main_app_dlg->GetMainServer()->UnLockChanges();

		if (ok)
			MessageBox("Выполнено", "OK");
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedEnableExecuterButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

		bool ok = false;
		
		CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		if (device_mgr)
		{
			int cur_sel = executers_combo.GetCurSel();
			
			if ( cur_sel != CB_ERR )
			{
				int executer_ord = executers_ordinals[cur_sel];
                device_mgr->GetDevices()[executer_ord]->GetExecuter()->RegardDevice();
                
				ok = true;				
            }
        }

        main_app_dlg->GetMainServer()->UnLockChanges();

		if (ok)
			MessageBox("Выполнено", "OK");
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedSetStateButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

		ERemoteDeviceResponse state_set = FALSE_RESPONSE;

		CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		if (device_mgr)
		{
			int cur_sel = sensors_combo.GetCurSel();
			int sensor_state_sel = sensor_states_combo.GetCurSel();
			
			if ( (cur_sel != CB_ERR) && (sensor_state_sel != CB_ERR) )
			{
				int sensor_ord = sensors_ordinals[cur_sel];				
				
                CString sensor_value_parameter;
                sensor_value_ed.GetWindowText(sensor_value_parameter);
                unsigned sensor_value = atol(sensor_value_parameter);

                ESensorState new_sensor_state = EncodeSensorState((ESensorStateEnum)sensor_state_sel, sensor_value); 

                state_set = device_mgr->GetDevices()[sensor_ord]->GetSensor()->SetSensorState(new_sensor_state);

				if ( state_set == TRUE_RESPONSE )
                {
                    std::string msg = "В РУЧНОМ РЕЖИМЕ установлено новое состояние : " + GetSensorStateStr(new_sensor_state, true);
                    msg += ", для сенсора: " + device_mgr->GetDevices()[sensor_ord]->GetName();
                    
                    bool in_emulated_state = device_mgr->GetDevices()[sensor_ord]->IsSensorInEmulatedState();

                    if ( in_emulated_state )
                        msg += " (режим эмуляции!!!)";
                    
                    main_app_dlg->LogMessage(msg, in_emulated_state);
                }
			}
		}

        main_app_dlg->GetMainServer()->UnLockChanges();

		if (state_set == TRUE_RESPONSE)
			MessageBox("Установлено", "OK");
		else
		{
			if (state_set == FALSE_RESPONSE)
				MessageBox("Ошибка установки!!!", "Ошибка");
			else
				MessageBox("Нет ответа от устройства!!!", "Ошибка");
		}
	}
}

void CQuestMainAppLowControlDlg::OnBnClickedSetEmulationOnButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

		bool ok = false;

		CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		if (device_mgr)
		{
			int cur_sel = sensors_combo.GetCurSel();
			int sensor_state_sel = sensor_states_combo.GetCurSel();
			
			if ( (cur_sel != CB_ERR) && (sensor_state_sel != CB_ERR) )
			{
				int sensor_ord = sensors_ordinals[cur_sel];		
                
                CString sensor_value_parameter;
                sensor_value_ed.GetWindowText(sensor_value_parameter);
                unsigned sensor_value = atol(sensor_value_parameter);

                ESensorState new_sensor_state = EncodeSensorState((ESensorStateEnum)sensor_state_sel, sensor_value); 
                
                device_mgr->GetDevices()[sensor_ord]->GetSensor()->ForceState(new_sensor_state);

				ok = true;				
            }
        }

        main_app_dlg->GetMainServer()->UnLockChanges();

		if (ok)
			MessageBox("Установлено", "OK");
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedSetEmulationOffButton()
{
	if ( main_app_dlg->GetMainServer() )
	{
		main_app_dlg->GetMainServer()->LockChanges();

		bool ok = false;
		
		CRemoteDevicesManager* device_mgr = main_app_dlg->GetMainServer()->GetDeviceManager();

		if (device_mgr)
		{
			int cur_sel = sensors_combo.GetCurSel();
			
			if ( cur_sel != CB_ERR )
			{
				int sensor_ord = sensors_ordinals[cur_sel];		
                
                bool should_set_activated_state = (device_mgr->GetDevices()[sensor_ord]->IsActivated() == TRUE_RESPONSE);
                device_mgr->GetDevices()[sensor_ord]->GetSensor()->UnForceState(should_set_activated_state);

				ok = true;				
            }
        }

        main_app_dlg->GetMainServer()->UnLockChanges();

		if (ok)
			MessageBox("Установлено", "OK");
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedIncTimeButton()
{
	if ( main_app_dlg->GetMainServer() )
    {
        CString time_parameter;
        time_increase_ed.GetWindowText(time_parameter);

        long time_ms = atol(time_parameter) * 1000;

        if ( time_ms > 0 )
        {
            main_app_dlg->GetMainServer()->IncreaseQuestTimeMs(time_ms);
            main_app_dlg->CalculateRestTimeAlerts();

            char tmp_buffer[255];
            sprintf(tmp_buffer, "%u", time_ms / 1000);

            std::string msg = "В РУЧНОМ РЕЖИМЕ время увеличено на " + std::string(tmp_buffer);
            msg += " секунд";
            main_app_dlg->LogMessage(msg);
        }
    }
}

void CQuestMainAppLowControlDlg::OnBnClickedDecTimeButton()
{
	if ( main_app_dlg->GetMainServer() )
    {
        CString time_parameter;
        time_decrease_ed.GetWindowText(time_parameter);

        long time_ms = atol(time_parameter) * 1000;

        if ( time_ms > 0 )
        {
            main_app_dlg->GetMainServer()->DecreaseQuestTimeMs(time_ms);
            main_app_dlg->CalculateRestTimeAlerts();

            char tmp_buffer[255];
            sprintf(tmp_buffer, "%u", time_ms / 1000);

            std::string msg = "В РУЧНОМ РЕЖИМЕ время уменьшено на " + std::string(tmp_buffer);
            msg += " секунд";
            main_app_dlg->LogMessage(msg);
        }
    }
}

