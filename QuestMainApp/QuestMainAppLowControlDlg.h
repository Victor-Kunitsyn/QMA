#pragma once

#include "resource.h"
#include "QuestServer.h"

class CQuestMainAppDlg;

class CQuestMainAppLowControlDlg : public CPropertyPage
{
public:
    enum { IDD = IDD_QUESTMAINAPPLOWCONTROL_DIALOG };
	
    CQuestMainAppLowControlDlg(const CQuestMainAppDlg* main_app_dlg_p);
	virtual ~CQuestMainAppLowControlDlg();

	afx_msg void OnBnClickedExecuteButton();
    afx_msg void OnBnClickedDisableExecuterButton();
    afx_msg void OnBnClickedEnableExecuterButton();

	afx_msg void OnBnClickedSetStateButton();
    afx_msg void OnBnClickedSetEmulationOnButton();
    afx_msg void OnBnClickedSetEmulationOffButton();

    afx_msg void OnBnClickedIncTimeButton();
	afx_msg void OnBnClickedDecTimeButton();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG*);

    afx_msg void OnTimer(UINT nIDEvent);

    void EnableDisableControls();
	void InitCombos();

protected:
    const CQuestMainAppDlg* main_app_dlg;

	CComboBox executers_combo;
	CEdit executer_parameter_ed;

	CComboBox sensors_combo;
	CEdit sensor_state_ed;
	CComboBox sensor_states_combo;
    CEdit sensor_value_ed;

	CEdit time_increase_ed;
	CEdit time_decrease_ed;

    std::vector<int> executers_ordinals;
	std::vector<int> sensors_ordinals;
	
	UINT controls_refresh_timer;

    bool combos_initialized;

    DECLARE_MESSAGE_MAP()
};
