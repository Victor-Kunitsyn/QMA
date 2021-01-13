#include "stdafx.h"
#include "QuestMainAppPropSheet.h"

CQuestMainAppPropSheet::CQuestMainAppPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd /*= NULL*/, UINT iSelectPage /*= 0*/) : CPropertySheet(pszCaption, pParentWnd, iSelectPage)																															
{
}

BEGIN_MESSAGE_MAP(CQuestMainAppPropSheet, CPropertySheet)
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

BOOL CQuestMainAppPropSheet::OnInitDialog()
{
    CPropertySheet::OnInitDialog();

    ModifyStyle( WS_SYSMENU, 0 );

    CWnd* ptr_button;
        
    ptr_button = GetDlgItem(ID_APPLY_NOW);

    if( ptr_button != NULL )
        ptr_button->ShowWindow(SW_HIDE);
   
    ptr_button = GetDlgItem(IDOK);

    if( ptr_button != NULL )
        ptr_button->ShowWindow(SW_HIDE);

    ptr_button = GetDlgItem(IDCANCEL);

    if( ptr_button != NULL )
        ptr_button->ShowWindow(SW_HIDE);

    ptr_button = GetDlgItem(IDHELP);

    if( ptr_button != NULL )
        ptr_button->ShowWindow(SW_HIDE);

    return TRUE;
}

BOOL CQuestMainAppPropSheet::PreTranslateMessage(MSG* pMsg)
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

void CQuestMainAppPropSheet::EnableTabControl(BOOL bEnable)
{
	CTabCtrl* pTab = GetTabControl();

	if ( pTab )
		pTab->EnableWindow(bEnable);
}

BOOL CQuestMainAppPropSheet::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if ( GetActiveIndex() == 1 ) //Scenarios page
    {
        WPARAM wParam = ( (zDelta < 0) ? SB_PAGEDOWN : SB_PAGEUP );
        GetActivePage()->SendMessage(WM_VSCROLL, wParam, NULL);
    }
    
    return CPropertySheet::OnMouseWheel(nFlags, zDelta, pt);
}

