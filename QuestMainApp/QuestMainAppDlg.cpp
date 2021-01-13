#include "stdafx.h"

#include <tlhelp32.h>

#include "QuestMainApp.h"
#include "QuestMainAppDlg.h"
#include "QuestMainAppPropSheet.h"
#include "QuestMainAppSecurityCheckDlg.h"
#include "QuestMainAppNumPlayersDlg.h"
#include "QuestMainAppLowControlDlg.h"
#include "QuestMainAppScenariosControlDlg.h"

#include "ArduinoExecuter.h"
#include "ArduinoSensor.h"
#include "TenvisIPCamExecuter.h"
#include "HTTPGetExecuter.h"
#include "HTTPGetSensor.h"
#include "DebugDevice.h"
#include "OutputWindowExecuter.h"

#include "check_security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const COLORREF red_color = RGB(255, 0, 0);
static const COLORREF blue_color = RGB(0, 0, 255);
static const COLORREF black_color = RGB(0, 0, 0);
static const COLORREF cyan_color = RGB(127, 255, 212);

const DWORD CQuestMainAppDlg::rest_minutes_to_alert[] = { 180, 150, 120, 90, 60, 30, 15, 5 };

#define MAX_ACTIVATIONS_NUM_WITHOUT_INTERNET 50

const static char QUEST_INI_FILE[] = "Quest.ini";
const static char QUEST_INI_FILE_PATTERN[] = "Quest%d.ini";

const static char SCENARIOS_INI_FILE[] = "Scenarios.ini";
const static char SCENARIOS_INI_FILE_PATTERN[] = "Scenarios%d.ini";

const static char CLIENT_INI_FILE_NAME[] = "Client.ini";

const static char QUEST_LOG_FILE[] = "Quest.log";

const static char ARDUINO_SENSOR_TYPE[] = "ARDUINO_SENSOR";
const static char ARDUINO_SENSOR_WITH_ACTIVATOR_TYPE[] = "ARDUINO_SENSOR_WITH_ACTIVATOR";
const static char ARDUINO_EXECUTER_TYPE[] = "ARDUINO_EXECUTER";
const static char TENVIS_IP_CAM_TYPE[] = "TENVIS_IP_CAM";
const static char HTTP_GET_EXECUTER_TYPE[] = "HTTP_GET_EXECUTER";
const static char HTTP_GET_SENSOR_TYPE[] = "HTTP_GET_SENSOR";

//for debugging kernel
const static char DEBUG_SENSOR_TYPE[] = "DEBUG_SENSOR";
const static char DEBUG_EXECUTER_TYPE[] = "DEBUG_EXECUTER";

const static char OUTPUT_WINDOW_EXECUTER_TYPE[] = "OUTPUT_WINDOW_EXECUTER";

CRemoteDevice* CreateRemoteDevice(const char* device_type,
	                              const std::string& ip, int port, const std::string& usr, const std::string& psw,
	                              int id, int request_threshold, const std::string& name)
{
	CRemoteDevice* ret = NULL;

	if (strcmp(device_type, ARDUINO_SENSOR_TYPE) == 0) ret = new CArduinoSensor(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, ARDUINO_SENSOR_WITH_ACTIVATOR_TYPE) == 0) ret = new CArduinoSensorWithActivator(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, ARDUINO_EXECUTER_TYPE) == 0) ret = new CArduinoExecuter(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, TENVIS_IP_CAM_TYPE) == 0) ret = new CTenvisIPCamExecuter(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, HTTP_GET_EXECUTER_TYPE) == 0) ret = new CHTTPGetExecuter(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, HTTP_GET_SENSOR_TYPE) == 0) ret = new CHTTPGetSensor(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, DEBUG_SENSOR_TYPE) == 0) ret = new CDebugSensor(ip, port, usr, psw, id, request_threshold, name);
	else if (strcmp(device_type, DEBUG_EXECUTER_TYPE) == 0) ret = new CDebugExecuter(ip, port, usr, psw, id, request_threshold, name);
        else if (strcmp(device_type, OUTPUT_WINDOW_EXECUTER_TYPE) == 0) ret = new COutputWindowExecuter(ip, port, usr, psw, id, request_threshold, name);

	return ret;
}

static CString GetPathToSecurityFile(const CString& security_code)
{
	return ("/Security/" + security_code + ".txt");
}

static CString GetPathToStatisticsFile(const CString& security_code)
{
	return ("/Statistics/" + security_code + ".txt");
}

#define START_REINIT_LOW_CONTROLS		WM_USER + 1000
#define FINISH_REINIT_LOW_CONTROLS  	WM_USER + 1001

CQuestMainAppDlg::CQuestMainAppDlg(CQuestMainAppPropSheet* owner_p) : CPropertyPage(CQuestMainAppDlg::IDD),
                                                                      main_server(NULL),
                                                                      controls_refresh_timer(0),
                                                                      publish_log_message_timer(0),
                                                                      activations_without_internet_connection(0),
                                                                      super_user_security_code(false),
                                                                      owner_ptr(owner_p),
                                                                      scenarios_cntrl_dlg(NULL),
                                                                      low_cntrl_dlg(NULL)

{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	rest_time_alert_done = new BOOL[sizeof(rest_minutes_to_alert) / sizeof(DWORD)];

	char exe_file_path[1024];
	int num_cpy = ::GetModuleFileName(NULL, exe_file_path, sizeof(exe_file_path));

	if (num_cpy)
	{
		path_to_quest_ini = exe_file_path;
		
        unsigned name_beg = path_to_quest_ini.find_last_of("\\");
        path_to_quest_ini.erase(name_beg + 1);

		executable_name = exe_file_path + name_beg + 1;
        
        if ( !BuildPaths() )
			LogMessage("ОШИБКА файлов инициализации", true);
	}
	else
		LogMessage("ОШИБКА GetModuleFileName", true);
}

CQuestMainAppDlg::~CQuestMainAppDlg()
{
	delete[] rest_time_alert_done;

	delete scenarios_cntrl_dlg;
	delete low_cntrl_dlg;
}

LRESULT CQuestMainAppDlg::StartReinit(WPARAM, LPARAM)
{
	if (owner_ptr)
	{
		owner_ptr->SetActivePage(0);

		if (owner_ptr->GetPageCount() > 1)
			owner_ptr->RemovePage(1);

		if (owner_ptr->GetPageCount() > 1)
			owner_ptr->RemovePage(1);

		delete scenarios_cntrl_dlg;
		scenarios_cntrl_dlg = NULL;

		delete low_cntrl_dlg;
		low_cntrl_dlg = NULL;
	}

	return 0;
}

LRESULT CQuestMainAppDlg::FinishReinit(WPARAM, LPARAM)
{
	if (owner_ptr)
	{
		if (scenarios_cntrl_dlg == NULL)
			scenarios_cntrl_dlg = new CQuestMainAppScenariosControlDlg(this);

		if (low_cntrl_dlg == NULL)
			low_cntrl_dlg = new CQuestMainAppLowControlDlg(this);

		owner_ptr->AddPage(scenarios_cntrl_dlg);
		owner_ptr->AddPage(low_cntrl_dlg);
	}

	return 0;
}

void CQuestMainAppDlg::OnStartReInit()
{
	::PostMessage(*this, START_REINIT_LOW_CONTROLS, 0, 0);
}

void CQuestMainAppDlg::OnFinishReInit()
{
	::PostMessage(*this, FINISH_REINIT_LOW_CONTROLS, 0, 0);
}

static bool IsFileExist(const char* f_name)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = FindFirstFileA(f_name, &FindFileData);

	bool ret = hFind != INVALID_HANDLE_VALUE && ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

	::FindClose(hFind);

	return ret;
}

bool CQuestMainAppDlg::BuildPaths()
{
	bool ret = true;

	full_paths_to_ini_files.clear();
	full_paths_to_scenarios_ini_files.clear();

	char file_name_buf[1024];

	for (int i = 0; ; i++)
	{
		if (i > 0)
			sprintf(file_name_buf, QUEST_INI_FILE_PATTERN, i);
		else
			sprintf(file_name_buf, QUEST_INI_FILE);

		std::string full_quest_ini_path = path_to_quest_ini + file_name_buf;

		if (IsFileExist(full_quest_ini_path.c_str()))
		{
			full_paths_to_ini_files.push_back(full_quest_ini_path);

			if (i > 0)
				sprintf(file_name_buf, SCENARIOS_INI_FILE_PATTERN, i);
			else
				sprintf(file_name_buf, SCENARIOS_INI_FILE);

			std::string full_scenarios_ini_path = path_to_quest_ini + file_name_buf;

			if (IsFileExist(full_scenarios_ini_path.c_str()))
				full_paths_to_scenarios_ini_files.push_back(full_scenarios_ini_path);
			else
				full_paths_to_scenarios_ini_files.push_back("");
		}
		else
			break;
	}

	return ret;
}

void CQuestMainAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TIMER_WINDOW, timer_wnd);
}

BEGIN_MESSAGE_MAP(CQuestMainAppDlg, CPropertyPage)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
    ON_BN_CLICKED(IDC_START_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedStartSystemButton)
    ON_BN_CLICKED(IDC_CHECK_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedCheckSystemButton)
    ON_BN_CLICKED(IDC_EXPERT_LEVEL, &CQuestMainAppDlg::OnBnClickedExpertLevel)
    ON_BN_CLICKED(IDC_EXTENDED_LOG, &CQuestMainAppDlg::OnBnClickedExtendedLog)
    ON_BN_CLICKED(IDC_ACTIVATE_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedActivateButton)
    ON_BN_CLICKED(IDC_DEACTIVATE_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedDeActivateSystemButton)
    ON_BN_CLICKED(IDC_PAUSE_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedPauseSystemButton)
    ON_BN_CLICKED(IDC_RESUME_SYSTEM_BUTTON, &CQuestMainAppDlg::OnBnClickedResumeSystemButton)
    ON_BN_CLICKED(IDCANCEL, &CQuestMainAppDlg::OnBnClickedCancel)
    ON_MESSAGE(START_REINIT_LOW_CONTROLS, StartReinit)
    ON_MESSAGE(FINISH_REINIT_LOW_CONTROLS, FinishReinit)
END_MESSAGE_MAP()

void CQuestMainAppDlg::GetActivateStatus(bool& can_activate_system, bool& can_activate_next_stage) const
{
	can_activate_system = false;
	can_activate_next_stage = false;

	if (main_server)
	{
		EServerStatus server_status = main_server->GetStatus();

		if (server_status != SERVER_STOPPED)
		{
			if (server_status == SERVER_RUNNING)
				can_activate_system = true;
			else
			{
				if ((server_status == SERVER_ACTIVATED) || (server_status == SERVER_PAUSED))
				{
					if ((main_server->GetCurrentStageNum() + 1) < main_server->GetStagesNumber())
						can_activate_next_stage = true;
				}
			}
		}
	}
}

unsigned CQuestMainAppDlg::GetNumberOfExecutedInstances() const
{
    unsigned ret = 0;

    HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if ( hSnap != NULL )
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (::Process32First(hSnap, &pe32))
        {
            if (strcmp(pe32.szExeFile, executable_name.c_str()) == 0)
                ret++;
         
            while (::Process32Next(hSnap, &pe32))
            {
                if (strcmp(pe32.szExeFile, executable_name.c_str()) == 0)
                    ret++;
            }
        }
    }
    
    ::CloseHandle(hSnap);

    return ret;
}

bool CQuestMainAppDlg::CanPerformTotalChecking() const
{
    bool ret = false;

    if ( main_server )
    {
        if ( main_server->GetStatus() == SERVER_RUNNING )   
        {
            if ( GetNumberOfExecutedInstances() == 1 )
                ret = true;
        }
    }

    return ret;
}

void CQuestMainAppDlg::EnableDisableControls()
{
	if (main_server)
	{
		if (main_server->GetQuestName()[0] != 0x0)
		{
            CString prev_scenario_block_str; 
            GetDlgItem(IDC_QUEST_NAME)->GetWindowText(prev_scenario_block_str);

            char message_buf[100];
			sprintf(message_buf, "%s, блок сценариев %d", main_server->GetQuestName(), main_server->GetCurrentStageNum());
            CString cur_scenario_block_str = message_buf; 			
            
            if ( cur_scenario_block_str != prev_scenario_block_str )
                GetDlgItem(IDC_QUEST_NAME)->SetWindowText(cur_scenario_block_str);
		}

		EServerStatus server_status = main_server->GetStatus();

		::EnableWindow(*GetDlgItem(IDC_START_SYSTEM_BUTTON), (server_status == SERVER_STOPPED ? TRUE : (server_status == SERVER_RUNNING ? FALSE : FALSE)));
		::EnableWindow(*GetDlgItem(IDC_EXPERT_LEVEL), (server_status == SERVER_STOPPED ? FALSE : (server_status == SERVER_RUNNING ? TRUE : FALSE)));

        CButton* test_button = (CButton*)(GetDlgItem(IDC_CHECK_SYSTEM_BUTTON));

        if ( server_status == SERVER_STOPPED )
        {
			test_button->SetWindowText("Проверить");
			::EnableWindow(*test_button, FALSE);
        }
        else
        {
            if ( CanPerformTotalChecking() )
                test_button->SetWindowText("Проверить всю систему");
            else
                test_button->SetWindowText("Проверить сценарий");

            ::EnableWindow(*test_button, TRUE);
        }
        
        bool can_activate_system, can_activate_next_stage;
		GetActivateStatus(can_activate_system, can_activate_next_stage);

		CButton* activate_button = (CButton*)(GetDlgItem(IDC_ACTIVATE_SYSTEM_BUTTON));

		if (can_activate_system || can_activate_next_stage)
		{
			if (can_activate_system)
				activate_button->SetWindowText("Активировать систему");
			else
				activate_button->SetWindowText("Активировать след. блок");

			::EnableWindow(*activate_button, TRUE);
		}
		else
		{
			activate_button->SetWindowText("Активировать");
			::EnableWindow(*activate_button, FALSE);
		}

		::EnableWindow(*GetDlgItem(IDC_DEACTIVATE_SYSTEM_BUTTON), (server_status == SERVER_STOPPED ? FALSE : (server_status == SERVER_RUNNING ? FALSE : TRUE)));
		::EnableWindow(*GetDlgItem(IDC_PAUSE_SYSTEM_BUTTON), (server_status == SERVER_ACTIVATED ? TRUE : FALSE));
		::EnableWindow(*GetDlgItem(IDC_RESUME_SYSTEM_BUTTON), (server_status == SERVER_PAUSED ? TRUE : FALSE));

		CButton* level_check_box = (CButton*)(GetDlgItem(IDC_EXPERT_LEVEL));
		level_check_box->SetCheck((main_server->GetLevel() == ORDINARY ? FALSE : TRUE));

		unsigned bitmap_id = ((server_status == SERVER_ACTIVATED) ? IDB_LOCK_CLOSED_BITMAP : IDB_LOCK_OPENED_BITMAP);
		HBITMAP hBitmap = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(bitmap_id), IMAGE_BITMAP, 0, 0, LR_VGACOLOR | LR_LOADMAP3DCOLORS);
		HBITMAP prevBitmap = ((CStatic*)GetDlgItem(IDC_STATIC_PICTURE))->SetBitmap(hBitmap);
		::DeleteObject(prevBitmap);

		if (owner_ptr)
		{
			if (server_status == SERVER_STOPPED)
				owner_ptr->EnableTabControl(FALSE);
			else
				owner_ptr->EnableTabControl(TRUE);
		}
	}
}

// CQuestMainAppDlg message handlers

BOOL CQuestMainAppDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	timer_wnd.SetBackgroundColor(FALSE, cyan_color);

	main_server = new CMainServer(&CreateRemoteDevice, this);
	main_server->AddListener(this);

    CButton* log_check_box = (CButton*)(GetDlgItem(IDC_EXTENDED_LOG));
    log_check_box->SetCheck( (main_server->IsExtendedLogging() ? TRUE : FALSE) );
	EnableDisableControls();

	controls_refresh_timer = SetTimer(1, 500, 0);
	publish_log_message_timer = SetTimer(2, 600000, 0);

	((CEdit*)(GetDlgItem(IDC_MAIN_OUTPUT_WINDOW)))->SetLimitText(-1);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQuestMainAppDlg::OnCancel()
{
	KillTimer(controls_refresh_timer);
	KillTimer(publish_log_message_timer);

	if (main_server != NULL)
	{
		if ( (main_server->GetStatus() == SERVER_ACTIVATED) || (main_server->GetStatus() == SERVER_PAUSED) )
        {
			LogMessage("---------GAME END : EXTERNAL BREAK---------");
            LogNumberWithPattern(main_server->GetSystemActivationTime(), "GAME ID: %u");
            LogStatistics(EXTERNAL_BREAK_GT, main_server->GetPlayersNum(), main_server->GetElapsedTimeMin());
        }
		
		main_server->StopSystem();
		delete main_server;
		main_server = NULL;

		ProcessLogInfos(true);
        
		if (security_code != "")
		{
            while (log_info_str_file != "")
                ProcessLogInfoStrToFile();

			while (statistics_info_str != "")
			{
				WriteStatisticsToServer();

				if (statistics_info_str != "")
				{
					if (super_user_security_code)
					{
						MessageBox("Статистика не сохранена, проверьте подключение к интернету!!!", "Предупреждение");
						break;
					}
					else
						MessageBox("Проверьте подключение к интернету!!!", "Ошибка");
				}
			}
  
			if (GetNumberOfExecutedInstances() == 1)
			{
				memStream mem_stream;
				memStreamInit(&mem_stream);

				CString path_to_sec_file = GetPathToSecurityFile(security_code);
				bool file_not_exist;
				drop_box_accessor.ReadFile(path_to_sec_file, mem_stream, file_not_exist);

				if (ReleaseSecurityCode(mem_stream))
					drop_box_accessor.PutFile(path_to_sec_file, mem_stream);

				memStreamCleanup(&mem_stream);
			}
		}
	}

	CPropertyPage::OnCancel();

	if (owner_ptr)
		owner_ptr->EndDialog(IDCANCEL);
}

BOOL CQuestMainAppDlg::PreTranslateMessage(MSG* pMsg)
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

#define MAX_STRING_SIZE_TO_LOG 5000

void CQuestMainAppDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == controls_refresh_timer)
	{
		ProcessLogInfos();
		EnableDisableControls();
	}
	else
	{
		if (nIDEvent == publish_log_message_timer)
		{
            main_dlg_stat_cs.Enter();

            if ( statistics_info_str.size() > 0 )
                WriteStatisticsToServer();

            main_dlg_stat_cs.Leave();

            if ( log_info_str_file.size() > MAX_STRING_SIZE_TO_LOG )
                ProcessLogInfoStrToFile();
		}
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CQuestMainAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPropertyPage::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CQuestMainAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CQuestMainAppDlg::LogMessageInternal(const std::string& msg, const SYSTEMTIME& msg_time, bool need_alert)
{
	char time_buffer[50];
	sprintf(time_buffer, "(%.2d:%.2d:%.2d, %.2d/%.2d/%d)   ", msg_time.wHour, msg_time.wMinute, msg_time.wSecond, msg_time.wDay, msg_time.wMonth, msg_time.wYear);

	CEdit* main_output = (CEdit*)(GetDlgItem(IDC_MAIN_OUTPUT_WINDOW));

	int last_pos = main_output->GetWindowTextLength();
	main_output->SetSel(last_pos, last_pos);

	std::string text_to_display = std::string(time_buffer) + msg;
	main_output->ReplaceSel(text_to_display.c_str());
	main_output->ReplaceSel("\r\n");

	if (need_alert)
		::Beep(888, 100);

	log_info_str_file += text_to_display;
	log_info_str_file += 0x0D; log_info_str_file += 0x0A;
}

static void WriteXMLtoString(void* p_string, const char* buffer, int len)
{
	std::string& str = *((std::string*)p_string);
	str.append(buffer, len);
}

void CQuestMainAppDlg::LogRemainTimeInternal(DWORD time_ms)
{
	if (main_server == NULL)
		return;

	timer_wnd.SetSel(0, -1);

	static const LONG timer_text_size = 350;
	static const LONG info_text_size = 280;

	if ((time_ms != DWORD(-1)) && (main_server->GetStatus() == SERVER_ACTIVATED))
	{
		DWORD time_sec = time_ms / 1000;

		div_t div_result = div(time_sec, 3600);
		int hour_rem = div_result.quot;
		div_result = div(div_result.rem, 60);
		int min_rem = div_result.quot;
		int sec_rem = div_result.rem;

		char time_buffer[30];
		sprintf(time_buffer, "%.2d : %.2d : %.2d", hour_rem, min_rem, sec_rem);

		timer_wnd.AddText(time_buffer, TRUE, FALSE, &red_color, &timer_text_size);

		DWORD min_rest = time_sec / 60;

		for (int i = 0; i < sizeof(rest_minutes_to_alert) / sizeof(DWORD); i++)
		{
			if (min_rest <= rest_minutes_to_alert[i])
			{
				if (!rest_time_alert_done[i])
				{
					const static char ON_TIME_SECT[] = "ON_TIME_ACTION";
					const static char ON_TIME_URL_PATTERN_KEY[] = "URL_PATTERN";

					char output_buffer[1024];
					::GetPrivateProfileString(ON_TIME_SECT, ON_TIME_URL_PATTERN_KEY, "", output_buffer, sizeof(output_buffer), (path_to_quest_ini + CLIENT_INI_FILE_NAME).c_str());

					if (output_buffer[0] != 0x0)
					{
						CURLDownloaderLight downloader;
						downloader.UseProxy("", "");
						char pb_url[255];
						sprintf(pb_url, output_buffer, min_rest);
						downloader.SetURL(pb_url);
						std::string o_result_xml;
						downloader.GetData(WriteXMLtoString, &o_result_xml);
					}

					rest_time_alert_done[i] = TRUE;
					break;
				}
			}
			else
				break;
		}
	}
	else
	{
		if (main_server->GetStatus() == SERVER_PAUSED)
			timer_wnd.AddText("     PAUSED", FALSE, FALSE, &blue_color, &info_text_size);
		else
			timer_wnd.AddText("DEACTIVATED", FALSE, FALSE, &blue_color, &info_text_size);
	}
}

#define MAX_LOG_MESAGGES 30

void CQuestMainAppDlg::LogMessage(const std::string& msg, bool need_alert /*= false*/) const
{
	main_dlg_cs.Enter();

	SLogInfo log_info(msg, need_alert);
	log_infos.push_back(log_info);

	if (log_infos.size() > MAX_LOG_MESAGGES)
		log_infos.erase(log_infos.begin());

	main_dlg_cs.Leave();
}

void CQuestMainAppDlg::LogRemainTime(DWORD time_ms) const
{
	main_dlg_cs.Enter();

	std::vector<SLogInfo>::iterator log_infos_iterator;

	for (log_infos_iterator = log_infos.begin(); log_infos_iterator != log_infos.end(); ++log_infos_iterator)
	{
		if (!(log_infos_iterator->is_for_message))
		{
			log_infos.erase(log_infos_iterator); //deletes previous time message if any exists
			break;
		}
	}

	SLogInfo log_info(time_ms);
	log_infos.push_back(log_info);

	main_dlg_cs.Leave();
}

void CQuestMainAppDlg::WriteStatisticsToServer()
{
    main_dlg_stat_cs.Enter();

    if ( (security_code != "") && (statistics_info_str != "") )
    {
        CString path_to_statistics_file = GetPathToStatisticsFile(security_code);

        memStream mem_stream;
        memStreamInit(&mem_stream);

        bool file_not_exist;
        bool ret = drop_box_accessor.ReadFile(path_to_statistics_file, mem_stream, file_not_exist);

        if (ret || file_not_exist)
        {
            if (mem_stream.data && (mem_stream.size > 0))
            {
                if (mem_stream.data[mem_stream.size - 1] == 0x0)
                {
                    mem_stream.size--;
                    mem_stream.cursor--;
                }
            }

            memStreamWrite(statistics_info_str.c_str(), 1, statistics_info_str.length(), &mem_stream);

            mem_stream.cursor = 0;
            ret = drop_box_accessor.PutFile(path_to_statistics_file, mem_stream);

            if (ret)
                statistics_info_str = "";
        }

        memStreamCleanup(&mem_stream);   
    }

    main_dlg_stat_cs.Leave();
}

void CQuestMainAppDlg::LogStatistics(EEndGameType game_type, unsigned players_num, DWORD game_duration) const 
{
    SYSTEMTIME game_end_time;
    ::GetLocalTime(&game_end_time);

    char message_buffer[100];
    sprintf(message_buffer, "%.2d/%.2d/%d,%u,%u,%u", game_end_time.wDay, game_end_time.wMonth, game_end_time.wYear, game_type, players_num, game_duration);

    main_dlg_stat_cs.Enter();

    statistics_info_str += message_buffer;
    statistics_info_str += 0x0D; statistics_info_str += 0x0A;

    main_dlg_stat_cs.Leave();
}

void CQuestMainAppDlg::ProcessLogInfos(bool process_all /*= false*/)
{
	SLogInfo info_to_log;
	bool need_log = false;

	main_dlg_cs.Enter();

	if (log_infos.size() > 0)
	{
		info_to_log = log_infos[0];
		log_infos.erase(log_infos.begin());
		need_log = true;
	}

	main_dlg_cs.Leave();

	if (need_log)
	{
		if (info_to_log.is_for_message)
            LogMessageInternal(info_to_log.msg, info_to_log.message_time, info_to_log.need_alert);
		else
			LogRemainTimeInternal(info_to_log.time_ms);

		if (process_all)
			ProcessLogInfos(true);
	}
}

void CQuestMainAppDlg::ProcessLogInfoStrToFile()
{
	if ((security_code != "") && (log_info_str_file != ""))
	{
        std::string path_to_log_file = path_to_quest_ini + QUEST_LOG_FILE;

        CFile* log_file = NULL;

        TRY
        {
            log_file = new CFile( path_to_log_file.c_str(), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite );
        }
        CATCH( CFileException, pEx )
        {
            // Simply show an error message to the user.
            pEx->ReportError();
        }
        AND_CATCH(CMemoryException, pEx)
        {
            // We can't recover from this memory exception, so we'll
            // just terminate the app without any cleanup. Normally, 
            // an application should do everything it possibly can to
            // clean up properly and not call AfxAbort( ).
            AfxAbort( );
        }
        END_CATCH

        if ( log_file != NULL )   
        {
            log_file->SeekToEnd();
            log_file->Write(log_info_str_file.c_str(), log_info_str_file.size());

            log_file->Close( );
            delete log_file;

            log_info_str_file = "";
        } 
    }
}

void CQuestMainAppDlg::OnBnClickedStartSystemButton()
{
	CQuestMainAppSecurityCheckDlg sec_dlg(&security_code);

	if (sec_dlg.DoModal() == IDOK)
	{
		memStream mem_stream;
		memStreamInit(&mem_stream);

		CString path_to_sec_file = GetPathToSecurityFile(security_code);
		bool file_not_exist;
		drop_box_accessor.ReadFile(path_to_sec_file, mem_stream, file_not_exist);

		bool code_can_be_fixed, code_is_fixed;
		
		ESecurityCheckResult sec_result = CheckSecurityCode(security_code, mem_stream, super_user_security_code, code_can_be_fixed, code_is_fixed);

		if (sec_result != SEC_CODE_OK)
			security_code = "";

		switch (sec_result)
		{
		case SEC_CODE_OK:
			drop_box_accessor.PutFile(path_to_sec_file, mem_stream);
			main_server->StartSystem(full_paths_to_ini_files, full_paths_to_scenarios_ini_files);
			break;

		case SEC_CODE_INVALID:
			MessageBox("Введен несуществующий ключ или отсутствует подключение к интернету!!!", "Ошибка");
			break;

		case SEC_CODE_EXPIRED:
			MessageBox("Срок действия ключа истек!!!", "Ошибка");
			break;

		case SEC_CODE_BLOCKED:
			MessageBox("Ключ заблокирован!!!", "Ошибка");
			break;

		case SEC_CODE_SHARING_VIOLATION:
			MessageBox("Ключ уже используется другой копией программы!!!", "Ошибка");
			break;

		default:
			;
		}

		memStreamCleanup(&mem_stream);

		if (code_can_be_fixed) //security has been checked successfully and code can be mounted to this machine
		{
			if ( MessageBox("Осуществить привязку?", "Ключ может быть прикреплен к этому компьютеру", MB_YESNO | MB_DEFBUTTON2) == IDYES)
			{
				drop_box_accessor.ReadFile(path_to_sec_file, mem_stream, file_not_exist);
				
				if (MountSecurityCode(mem_stream))
				{
					drop_box_accessor.PutFile(path_to_sec_file, mem_stream);
					MessageBox("Выполнено", "Привязка");
				}
				else
					MessageBox("Ошибка", "Привязка");

				memStreamCleanup(&mem_stream);
			}
		}

		if (code_is_fixed) //security has been checked successfully and code can be unmounted from this machine
		{
			if (MessageBox("Осуществить открепление?", "Ключ может быть откреплен от этого компьютера", MB_YESNO | MB_DEFBUTTON2) == IDYES)
			{
				drop_box_accessor.ReadFile(path_to_sec_file, mem_stream, file_not_exist);

				if (UnMountSecurityCode(mem_stream))
				{
					drop_box_accessor.PutFile(path_to_sec_file, mem_stream);
					MessageBox("Выполнено", "Открепление");
				}
				else
					MessageBox("Ошибка", "Открепление");

				memStreamCleanup(&mem_stream);
			}
		}
	}
}

void CQuestMainAppDlg::OnBnClickedCheckSystemButton()
{
    if ( main_server )
    {
        if ( CanPerformTotalChecking() )
            main_server->CheckTotal();
        else
            main_server->CheckSystem();
    }
}

void CQuestMainAppDlg::OnBnClickedExpertLevel()
{
    main_server->SwitchExpertLevel();
}

void CQuestMainAppDlg::OnBnClickedExtendedLog()
{
    if ( main_server )
    {
        bool log_flag = main_server->IsExtendedLogging();
        main_server->SetExtendedLogging(!log_flag);
    }
}

void CQuestMainAppDlg::OnBnClickedActivateButton()
{
	bool can_activate_system, can_activate_next_stage;
	GetActivateStatus(can_activate_system, can_activate_next_stage);

	if (can_activate_system)
	{
		unsigned num_players;
		CQuestMainAppNumPlayersDlg num_players_dlg(&num_players);

		if (num_players_dlg.DoModal() == IDOK)
		{
			if ((num_players >= main_server->GetMinPlayersNum()) && (num_players <= main_server->GetMaxPlayersNum()))
			{
				main_server->SetPlayersNum(num_players);
				memStream mem_stream;
				memStreamInit(&mem_stream);

				CString path_to_sec_file = GetPathToSecurityFile(security_code);
				bool file_not_exist;
				drop_box_accessor.ReadFile(path_to_sec_file, mem_stream, file_not_exist);

				bool code_can_be_fixed, code_is_fixed;

				ESecurityCheckResult sec_result = CheckSecurityCode(security_code, mem_stream, super_user_security_code, code_can_be_fixed, code_is_fixed);
                
                if ( sec_result == SEC_CODE_OK )
				{
					drop_box_accessor.PutFile(path_to_sec_file, mem_stream);

                    main_server->ActivateSystem();
                    CalculateRestTimeAlerts();
                    
                    activations_without_internet_connection = 0;
				}
				else
				{
					if ( sec_result == SEC_CODE_INVALID ) //no internet connection
                    {
                        activations_without_internet_connection++;

                        if ( activations_without_internet_connection <= MAX_ACTIVATIONS_NUM_WITHOUT_INTERNET )
                        {
                            char message_buf[1024];
                            sprintf(message_buf, "Осталось активаций без подключения к интернету: %u", MAX_ACTIVATIONS_NUM_WITHOUT_INTERNET - activations_without_internet_connection);
                            MessageBox("Отсутствует подключение к интернету!!!", message_buf);
                
                            main_server->ActivateSystem();
                            CalculateRestTimeAlerts();
                        }
                        else
                        {
                            activations_without_internet_connection--;
                            MessageBox("Превышено максимальное число активаций без подключения к интернету!!!", "Ошибка");
                        }
                    }
                    else
                        MessageBox("Ключ заблокирован!!!", "Ошибка");
				}

				memStreamCleanup(&mem_stream);
			}
			else
			{
				char message_buf[1024];
				sprintf(message_buf, "Введите валидное число игроков от %u до %u", main_server->GetMinPlayersNum(), main_server->GetMaxPlayersNum());
				MessageBox(message_buf, "Ошибка");
			}
		}
	}
	else
	{
		if (can_activate_next_stage)
		{
			main_server->ActivateNextStage();
		}
	}
}

void CQuestMainAppDlg::CalculateRestTimeAlerts() const
{
	for (int i = 0; i < sizeof(rest_minutes_to_alert) / sizeof(DWORD); i++)
	{
		rest_time_alert_done[i] = (main_server->GetQuestTimeMs() / 1000 / 60 < rest_minutes_to_alert[i] ? TRUE : FALSE);
	}
}

void CQuestMainAppDlg::OnBnClickedDeActivateSystemButton()
{
	LogMessage("---------GAME END : EXTERNAL BREAK---------");
    LogNumberWithPattern(main_server->GetSystemActivationTime(), "GAME ID: %u");
    LogStatistics(EXTERNAL_BREAK_GT, main_server->GetPlayersNum(), main_server->GetElapsedTimeMin());
	
	main_server->DeActivateSystem();
}

void CQuestMainAppDlg::OnBnClickedPauseSystemButton()
{
	main_server->PauseSystem();
}

void CQuestMainAppDlg::OnBnClickedResumeSystemButton()
{
	main_server->ResumeSystem();
}
void CQuestMainAppDlg::OnBnClickedCancel()
{
	OnCancel();
}
