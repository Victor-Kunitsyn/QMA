#include "stdafx.h"

#include "QuestMainAppOutputWindowDlg.h"

BEGIN_MESSAGE_MAP(CQuestMainAppOutputWindowDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CQuestMainAppOutputWindowDlg::CQuestMainAppOutputWindowDlg() : CDialog(), controls_refresh_timer(0)
{
}

CQuestMainAppOutputWindowDlg::~CQuestMainAppOutputWindowDlg()
{
    DestroyWindow();
}

void CQuestMainAppOutputWindowDlg::OnCancel()
{	
    KillTimer(controls_refresh_timer);
    CDialog::OnCancel();
}

void CQuestMainAppOutputWindowDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OUTPUT_WINDOW_TEXT, output_text);
}
    
BOOL CQuestMainAppOutputWindowDlg::OnInitDialog()
{
   BOOL ret = CDialog::OnInitDialog();
   
   text = "";
   output_text.SetWindowText("");
   
   controls_refresh_timer = SetTimer(1, 500, 0);
   
   return ret;
}

void CQuestMainAppOutputWindowDlg::SetText(const CString& str)
{
    output_wnd_cs.Enter();

    text = str;

    output_wnd_cs.Leave();
}

void CQuestMainAppOutputWindowDlg::OnTimer(UINT nIDEvent)
{
	EnableDisableControls();
}

void CQuestMainAppOutputWindowDlg::EnableDisableControls()
{
    if( ::IsWindow(output_text.m_hWnd) )
    {
        output_wnd_cs.Enter();

        output_text.SetWindowText(text);

        output_wnd_cs.Leave();
    }
}



