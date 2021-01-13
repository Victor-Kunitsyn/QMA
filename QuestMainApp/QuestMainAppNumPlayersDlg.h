#pragma once

#include "resource.h"

class CQuestMainAppNumPlayersDlg : public CDialog
{
public :
    enum { IDD = IDD_QUESTMAINAPPNUMPLAYERS_DIALOG };

    CQuestMainAppNumPlayersDlg(unsigned* ptr_num_players_p) : CDialog(CQuestMainAppNumPlayersDlg::IDD), ptr_num_players(ptr_num_players_p) {}
    virtual ~CQuestMainAppNumPlayersDlg() {}

protected:
    virtual void OnOK();

protected:
    unsigned* ptr_num_players;
};

