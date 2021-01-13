#pragma once

#include "resource.h"

#include "QuestCommon.h"

class CQuestMainAppOutputWindowDlg : public CDialog
{
public :
    enum { IDD = IDD_QUESTMAINAPP_OUPUT_DIALOG };

    CQuestMainAppOutputWindowDlg();
    virtual ~CQuestMainAppOutputWindowDlg();

    BOOL Create(CWnd* parent = NULL) { return CDialog::Create(CQuestMainAppOutputWindowDlg::IDD, parent); }

    void SetText(const CString& str);

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);

    afx_msg void OnTimer(UINT nIDEvent);

    virtual void OnCancel();

    void EnableDisableControls();

protected:
    CString text;
    CStatic output_text;

    UINT controls_refresh_timer;

    mutable CScopedCriticalSection output_wnd_cs;

    DECLARE_MESSAGE_MAP()
};


