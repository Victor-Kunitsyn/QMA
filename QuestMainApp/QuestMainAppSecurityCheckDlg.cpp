#include "stdafx.h"

#include "QuestMainAppSecurityCheckDlg.h"

void CQuestMainAppSecurityCheckDlg::OnOK()
{
    if ( ptr_sec_code )
    {
        CWnd* sec_edit = GetDlgItem(IDC_SECURITY_CODE_EDIT);
        sec_edit->GetWindowText(*ptr_sec_code);
    }

    CDialog::OnOK();
}
