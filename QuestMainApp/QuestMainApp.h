#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CQuestMainAppApp:
// See QuestMainApp.cpp for the implementation of this class
//

class CQuestMainAppApp : public CWinApp
{
public:
	CQuestMainAppApp();
	virtual ~CQuestMainAppApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CQuestMainAppApp theApp;