#pragma once

#include "resource.h"

class CQuestMainAppSecurityCheckDlg : public CDialog
{
public :
    enum { IDD = IDD_QUESTMAINAPPSECURITY_DIALOG };

    CQuestMainAppSecurityCheckDlg(CString* ptr_sec_code_p) : CDialog(CQuestMainAppSecurityCheckDlg::IDD), ptr_sec_code(ptr_sec_code_p) {}
    virtual ~CQuestMainAppSecurityCheckDlg() {}

protected:
    virtual void OnOK();

protected:
    CString* ptr_sec_code;
};

