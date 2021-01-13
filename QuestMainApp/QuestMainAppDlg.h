#pragma once

#include "MyRichEdit.h"
#include "QuestServer.h"
#include "DropBoxAccessor.h"

class CQuestMainAppPropSheet;
class CQuestMainAppScenariosControlDlg;
class CQuestMainAppLowControlDlg;

class CQuestMainAppDlg : public CPropertyPage, CLogger, CReInitServerDevicesListener
{
public:
    enum { IDD = IDD_QUESTMAINAPP_DIALOG };
	
    CQuestMainAppDlg(CQuestMainAppPropSheet* owner_p);
	virtual ~CQuestMainAppDlg();

    afx_msg void OnBnClickedStartSystemButton();
    afx_msg void OnBnClickedCheckSystemButton();
    afx_msg void OnBnClickedExpertLevel();
    afx_msg void OnBnClickedActivateButton();
    afx_msg void OnBnClickedDeActivateSystemButton();
    afx_msg void OnBnClickedPauseSystemButton();
    afx_msg void OnBnClickedResumeSystemButton();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedExtendedLog();

    const CMainServer* GetMainServer() const { return main_server; }

    void CalculateRestTimeAlerts() const;

    virtual void LogMessage(const std::string& msg, bool need_alert = false) const;
    virtual void LogRemainTime(DWORD time_ms) const;
    virtual void LogStatistics(EEndGameType game_type, unsigned players_num, DWORD game_duration) const;

    virtual void OnStartReInit();
    virtual void OnFinishReInit();
    
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG*);

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);

    void LogMessageInternal(const std::string& msg, const SYSTEMTIME& msg_time, bool need_alert);
    void LogRemainTimeInternal(DWORD time_ms);
    void ProcessLogInfos(bool process_all = false);
    void ProcessLogInfoStrToFile();
    void WriteStatisticsToServer();

    void EnableDisableControls();

    struct SLogInfo
    {
        SLogInfo() : is_for_message(true), need_alert(false), time_ms(-1) { ::GetLocalTime(&message_time); }
        SLogInfo(const std::string& msg_p, bool need_alert_p) : is_for_message(true), msg(msg_p), need_alert(need_alert_p), time_ms(-1) { ::GetLocalTime(&message_time); }
        SLogInfo(DWORD time_ms_p) : is_for_message(false), need_alert(false), time_ms(time_ms_p) { ::GetLocalTime(&message_time); }

        bool is_for_message;
        std::string msg;
        bool need_alert;
        DWORD time_ms;

        SYSTEMTIME message_time;
    };

    bool BuildPaths();

    void GetActivateStatus(bool& can_activate_system, bool& can_activate_next_stage) const;

    unsigned GetNumberOfExecutedInstances() const;
    
    bool CanPerformTotalChecking() const;

    LRESULT StartReinit(WPARAM, LPARAM);
    LRESULT FinishReinit(WPARAM, LPARAM);

protected:
	HICON m_hIcon;
    CMainServer* main_server;

    mutable MyRichEdit timer_wnd;

	mutable CScopedCriticalSection main_dlg_cs;
    mutable CScopedCriticalSection main_dlg_stat_cs;
    
    mutable std::vector<SLogInfo> log_infos;
    mutable std::string log_info_str_file;
    mutable std::string statistics_info_str;

	UINT controls_refresh_timer;
	UINT publish_log_message_timer;

    static const DWORD rest_minutes_to_alert[];
    BOOL* rest_time_alert_done;

    std::string path_to_quest_ini;
    std::string executable_name;
    std::vector<std::string> full_paths_to_ini_files;
    std::vector<std::string> full_paths_to_scenarios_ini_files;

    CString security_code;
    DWORD activations_without_internet_connection;
	bool super_user_security_code;
	CDropBoxAccessor drop_box_accessor;

	CQuestMainAppPropSheet* owner_ptr;

    CQuestMainAppScenariosControlDlg* scenarios_cntrl_dlg;
    CQuestMainAppLowControlDlg* low_cntrl_dlg;

    DECLARE_MESSAGE_MAP()
};
