#include "stdafx.h"

#include <string>
#include "MyRichEdit.h"

//to deal with old header file richedit.h where the following types are not declared
// EM_SETTEXTEX info; this struct is passed in the wparam of the message 
typedef struct _settextex_
{
    DWORD   flags;          // Flags (see the ST_XXX defines)           
    UINT    codepage;       // Code page for translation (CP_ACP for sys default,
                            //  1200 for Unicode, -1 for control default)   
} SETTEXTEX_;

void MyRichEdit::SetRichEditFont(bool bold, bool italic, const COLORREF* color /*= NULL*/, const LONG* txt_size /*= NULL*/)
{
    CHARFORMAT chFormat;
    chFormat.cbSize = sizeof(CHARFORMAT);

    if (color == NULL)
        chFormat.crTextColor = RGB(0,0,0);
    else
        chFormat.crTextColor = *color;

    chFormat.dwMask = (CFM_BOLD | CFM_ITALIC | CFM_COLOR);
    
    if ( txt_size )
    {
        chFormat.dwMask |=  CFM_SIZE;
        chFormat.yHeight = *txt_size;
    }

    chFormat.dwEffects = (bold ? CFE_BOLD : 0) | (italic ? CFE_ITALIC : 0);
    
    SetSelectionCharFormat(chFormat);
}

void MyRichEdit::AddText(const char * text, bool bold /*= TRUE*/, bool italic /*=FALSE*/, const COLORREF* color /*= NULL*/, const LONG* txt_size /*= NULL*/)
{
    COLORREF text_color = ( (color != NULL) ? (*color) : (def_text_color) );
    SetRichEditFont(bold, italic, &text_color, txt_size);
    ReplaceSel(text);
    SetRichEditFont(FALSE, FALSE);
}

void MyRichEdit::AddTextW(const wchar_t * text, bool bold /*= TRUE*/, bool italic /*= FALSE*/, const COLORREF* color /*= NULL*/, const LONG* txt_size /*= NULL*/)
{
    COLORREF text_color = ( (color != NULL) ? (*color) : (def_text_color) );
    SetRichEditFont(bold, italic, &text_color, txt_size);
    
    SETTEXTEX_ wParamOUT;
    
    wParamOUT.codepage = 1200; //unicode
    wParamOUT.flags = ST_SELECTION;

    LRESULT lResult = SendMessageW(m_hWnd, EM_SETTEXTEX, (WPARAM) &wParamOUT, (LPARAM)text); 

    SetRichEditFont(FALSE, FALSE);
}


void MyRichEdit::AddTextWithCaption(const char * caption, const char * value/* = NULL*/ )
{
    if (caption != NULL)
        AddText(caption, TRUE, FALSE );
    if (value != NULL)
    {
        std::string ins_itm = " ";
        ins_itm += value;
        ins_itm += "\r\n";
        AddText(ins_itm.c_str(), FALSE, FALSE);
    }
}

void MyRichEdit::AddTextWithCaptionW(const wchar_t * caption, const wchar_t * value /*= NULL*/ )
{
    if (caption != NULL)
        AddTextW(caption, TRUE, FALSE);
    
    if (value != NULL)
    {
        std::wstring ins_itm = L" ";
        ins_itm += value;
        AddTextW(value, FALSE, FALSE);
        AddText("\n", FALSE,FALSE);
    }
}

