#include "stdafx.h"

#include "QuestMainAppScenariosControlDlg.h"
#include "QuestMainAppDlg.h"

class CMyButton : public CButton
{
public:
    CMyButton() : CButton(), text_color(0), bk_color(DEF_BK_COLOR) {}
    virtual ~CMyButton() {}

    // Overridables (for owner draw only)
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

    void SetTextColor(DWORD color) { text_color = color; }
    void SetBkColor(DWORD color) { bk_color = color; }

protected:
    DWORD text_color;
    DWORD bk_color;
};

void CMyButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    // This code only works with buttons.
    if ( lpDrawItemStruct->CtlType == ODT_BUTTON )
    {
        UINT uStyle = DFCS_BUTTONPUSH;

        // If drawing selected, add the pushed style to DrawFrameControl.
        if ( lpDrawItemStruct->itemState & ODS_SELECTED )
            uStyle |= DFCS_PUSHED;

        // Draw the button frame.
        ::DrawFrameControl(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, DFC_BUTTON, uStyle);

        // Get the button's text.
        CString strText;
        GetWindowText(strText);

        COLORREF crOldBkColor = ::SetBkColor(lpDrawItemStruct->hDC, bk_color);
        HBRUSH bk_brush = ::CreateSolidBrush(bk_color);
        
        static const LONG delta = 1;
        RECT small_rc = {lpDrawItemStruct->rcItem.left + delta, lpDrawItemStruct->rcItem.top + delta, lpDrawItemStruct->rcItem.right - delta, lpDrawItemStruct->rcItem.bottom - delta};
        
        ::FillRect(lpDrawItemStruct->hDC, &small_rc, bk_brush);
        COLORREF crOldColor = ::SetTextColor(lpDrawItemStruct->hDC, text_color);

        ::DrawText(lpDrawItemStruct->hDC, strText, strText.GetLength(), &small_rc, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);

        ::SetTextColor(lpDrawItemStruct->hDC, crOldColor);
        ::DeleteObject(bk_brush);
        ::SetBkColor(lpDrawItemStruct->hDC, crOldBkColor);
    }
}

CQuestMainAppScenariosControlDlg::CQuestMainAppScenariosControlDlg(const CQuestMainAppDlg* main_app_dlg_p): CPropertyPage(CQuestMainAppScenariosControlDlg::IDD), 
                                                                                                            main_app_dlg(main_app_dlg_p), 
                                                                                                            controls_refresh_timer(0),
                                                                                                            scenarios_buttons(NULL),
																											m_fCreated(false)
{
}

CQuestMainAppScenariosControlDlg::~CQuestMainAppScenariosControlDlg()
{
    delete[] scenarios_buttons;
}

BEGIN_MESSAGE_MAP(CQuestMainAppScenariosControlDlg, CPropertyPage)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

BOOL CALLBACK EnumChildPos(HWND hWnd, LPARAM lParam)
{
	LPPOINT lpPoint = (LPPOINT)lParam;
	RECT rect;
	::GetWindowRect(hWnd, &rect);
	POINT ptLT = { rect.left,rect.top };
	::ScreenToClient(::GetParent(hWnd), &ptLT);
	ptLT.x -= lpPoint->x;
	ptLT.y -= lpPoint->y;
	::SetWindowPos(hWnd, 0, ptLT.x, ptLT.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	return TRUE;
}

BOOL CALLBACK EnumChildRects(HWND hWnd, LPARAM lParam)
{
	LPRECT lpRect = (LPRECT)lParam;
	RECT rect;
	::GetWindowRect(hWnd, &rect);
	POINT ptLT = { rect.left, rect.top };
	POINT ptRB = { rect.right, rect.bottom };
	::ScreenToClient(::GetParent(hWnd), &ptLT);
	::ScreenToClient(::GetParent(hWnd), &ptRB);
	if (ptLT.x < lpRect->left)
		lpRect->left = ptLT.x;
	if (ptLT.y < lpRect->top)
		lpRect->top = ptLT.y;
	if (ptRB.x > lpRect->right)
		lpRect->right = ptRB.x;
	if (ptRB.y > lpRect->bottom)
		lpRect->bottom = ptRB.y;

	return TRUE;
}

BOOL CQuestMainAppScenariosControlDlg::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

	EnableDisableControls();

	controls_refresh_timer = SetTimer(1, 500, 0);

    RECT m_rcClient;
    GetClientRect(&m_rcClient);

    m_fCreated = true;

    m_rcStartChilds.left = m_rcClient.right;
	m_rcStartChilds.top = m_rcClient.bottom;
	m_rcStartChilds.right = m_rcClient.right;
	m_rcStartChilds.bottom = m_rcClient.bottom;
	EnumChildWindows(m_hWnd, EnumChildRects, (LPARAM)&m_rcStartChilds);
	
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQuestMainAppScenariosControlDlg::OnCancel()
{	
    KillTimer(controls_refresh_timer);
    CPropertyPage::OnCancel();
}

BOOL CQuestMainAppScenariosControlDlg::PreTranslateMessage(MSG* pMsg)
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

void CQuestMainAppScenariosControlDlg::OnTimer(UINT nIDEvent)
{
	EnableDisableControls();
}

#define USER_CONTROL_VALUE 8000

void CQuestMainAppScenariosControlDlg::EnableDisableControls()
{
	if ( main_app_dlg->GetMainServer() )
	{
        if ( scenarios_buttons == NULL )
        {
            main_app_dlg->GetMainServer()->LockChanges();
            
            CScenariosManager* scenario_mgr = main_app_dlg->GetMainServer()->GetScenariosManager();

            if ( scenario_mgr )
            {
                int num_scenarios = scenario_mgr->GetScenariosNumber();
                
                if ( num_scenarios > 0 )
                {
                    scenarios_buttons = new CMyButton[num_scenarios];

                    RECT dialog_rc;
                    GetClientRect(&dialog_rc);
                    
                    static const long button_indent = 25;
					static const long button_height = 35;
 
					long button_width = (dialog_rc.right - dialog_rc.left - 3 * button_indent) / 2;

					for ( int i = 0; i < num_scenarios; i++ )
                    {
                        const SScenario* scenario = scenario_mgr->GetScenario(i);

						div_t div_res = div(i, 2);

						long button_top = button_indent + (button_height + button_indent) * div_res.quot;
						
						RECT button_rect;
						
						button_rect.top = button_top;
						button_rect.bottom = button_top + button_height;
						
						if ( div_res.rem == 0 )
						{
							button_rect.left = dialog_rc.left + button_indent;
							button_rect.right = button_rect.left + button_width;
						}
						else
						{
							button_rect.right = dialog_rc.right - button_indent;
							button_rect.left = button_rect.right - button_width;
						}

                        DWORD button_style = WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_PUSHBUTTON | BS_MULTILINE | BS_OWNERDRAW;
						
						scenarios_buttons[i].Create(scenario->name.c_str(), button_style, button_rect, this, USER_CONTROL_VALUE + i);
                        scenarios_buttons[i].SetTextColor(scenario->color);
                        scenarios_buttons[i].SetBkColor(scenario->bk_color);
                    }
                }
            }

            main_app_dlg->GetMainServer()->UnLockChanges();
        }
	}
}

LRESULT CQuestMainAppScenariosControlDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_COMMAND)
	{
		if (main_app_dlg->GetMainServer())
		{
			main_app_dlg->GetMainServer()->LockChanges();

			bool scenario_executed = false;
			
			CScenariosManager* scenario_mgr = main_app_dlg->GetMainServer()->GetScenariosManager();

			if (scenario_mgr)
			{
				int num_scenarios = scenario_mgr->GetScenariosNumber();

				if ((wParam >= USER_CONTROL_VALUE) && (wParam < (USER_CONTROL_VALUE + num_scenarios)))
				{
					scenario_executed = scenario_mgr->ExecuteScenario(wParam - USER_CONTROL_VALUE);
				}
			}

            main_app_dlg->GetMainServer()->UnLockChanges();

			if (scenario_executed)
				MessageBox("Выполнено", "OK");
			else
				MessageBox("Ошибка выполнения!!!", "Ошибка");
		}
	}
	
	return CPropertyPage::WindowProc(message, wParam, lParam);
}

void CQuestMainAppScenariosControlDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	RECT rcMain;
	GetClientRect(&rcMain);
	
	if ( (cy < m_rcStartChilds.bottom) && m_fCreated )
	{
		ModifyStyle(0, WS_VSCROLL, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		RECT rcChilds = { rcMain.right, rcMain.bottom, rcMain.right, rcMain.bottom };
		EnumChildWindows(m_hWnd, EnumChildRects, (LPARAM)&rcChilds);
		POINT pt = { 0, rcChilds.top - m_rcStartChilds.top };
		EnumChildWindows(m_hWnd, EnumChildPos, (LPARAM)&pt);
		SetScrollPos(SB_VERT, 0);
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_RANGE | SIF_PAGE;
		si.nMin = 0;
		si.nMax = (m_rcStartChilds.bottom + m_rcStartChilds.top) - rcMain.bottom;
		si.nPage = (si.nMax - si.nMin) / 10;
		
		si.nMax += si.nPage;
		
		SetScrollInfo(SB_VERT, &si);
	}
	else
	{
		RECT rcChilds = { rcMain.right, rcMain.bottom, rcMain.right, rcMain.bottom };
		EnumChildWindows(m_hWnd, EnumChildRects, (LPARAM)&rcChilds);
		POINT pt = { 0, rcChilds.top - m_rcStartChilds.top };
		EnumChildWindows(m_hWnd, EnumChildPos, (LPARAM)&pt);
		SetScrollPos(SB_VERT, 0);
		ModifyStyle(WS_VSCROLL, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CQuestMainAppScenariosControlDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	
    if ( !GetScrollInfo(SB_VERT, &si) )
        return;

	int nDelta = si.nPos;
	switch (nSBCode)
	{
	case SB_LINEUP:
		si.nPos -= 10;
		break;
	case SB_LINEDOWN:
		si.nPos += 10;
		break;
	case SB_PAGEUP:
		si.nPos -= si.nPage;
		break;
	case SB_PAGEDOWN:
		si.nPos += si.nPage;
		break;
	case SB_THUMBTRACK:
		si.nPos = si.nTrackPos;
		break;
	default:
		break;
	}

	si.fMask = SIF_POS;
	::SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
	::GetScrollInfo(m_hWnd, SB_VERT, &si);

	nDelta = si.nPos - nDelta;
	POINT pt = { 0, nDelta };
	EnumChildWindows(m_hWnd, EnumChildPos, (LPARAM)&pt);

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	
	CDialog::Invalidate();
	CDialog::UpdateWindow();
}

