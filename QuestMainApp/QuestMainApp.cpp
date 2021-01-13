#include "stdafx.h"

#include "QuestMainApp.h"

#include "QuestMainAppPropSheet.h"
#include "QuestMainAppDlg.h"

#include <QtCore/QCoreApplication>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CQuestMainAppApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CQuestMainAppApp::CQuestMainAppApp()
{
}

CQuestMainAppApp::~CQuestMainAppApp()
{
}

CQuestMainAppApp theApp;

BOOL CQuestMainAppApp::InitInstance()
{
	int argc_dummy = 0;
	QCoreApplication QtApp (argc_dummy, 0);
	
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

    // Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
    AfxInitRichEdit();

	CQuestMainAppPropSheet dlgPropertySheet("Quest Control Application");
    CQuestMainAppDlg main_dlg(&dlgPropertySheet);

    dlgPropertySheet.AddPage(&main_dlg);
    
    dlgPropertySheet.DoModal();

	return TRUE;
}
