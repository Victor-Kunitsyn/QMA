#pragma once

#include "resource.h"

class CQuestMainAppDlg;

class CMyButton;

class CQuestMainAppScenariosControlDlg : public CPropertyPage
{
public:
    enum { IDD = IDD_QUESTMAINAPPSCENARIOSCONTROL_DIALOG };
	
    CQuestMainAppScenariosControlDlg(const CQuestMainAppDlg* main_app_dlg_p);
	virtual ~CQuestMainAppScenariosControlDlg();

protected:
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG*);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT nIDEvent);

    void EnableDisableControls();

protected:
	const CQuestMainAppDlg* main_app_dlg;

    UINT controls_refresh_timer;

    CMyButton* scenarios_buttons;

protected:
	RECT m_rcStartChilds;
	bool m_fCreated;

    DECLARE_MESSAGE_MAP()
};
