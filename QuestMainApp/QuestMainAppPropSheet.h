#pragma once

class CQuestMainAppPropSheet : public CPropertySheet
{
public:
    explicit CQuestMainAppPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    virtual ~CQuestMainAppPropSheet() {}

public:
	void EnableTabControl(BOOL bEnable = TRUE);

protected:
    virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG*);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

    DECLARE_MESSAGE_MAP()
};
