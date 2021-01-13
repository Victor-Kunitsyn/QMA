#include "stdafx.h"

#include "QuestMainAppNumPlayersDlg.h"

void CQuestMainAppNumPlayersDlg::OnOK()
{
    if ( ptr_num_players )
    {
        CWnd* num_players_edit = GetDlgItem(IDC_NUM_PLAYERS_EDIT);
        CString num_players_str;
        num_players_edit->GetWindowText(num_players_str);
        *ptr_num_players = atoi(num_players_str);
    }

    CDialog::OnOK();
}
