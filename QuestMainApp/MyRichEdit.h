#pragma once

class MyRichEdit : public  CRichEditCtrl
{
public:
    MyRichEdit() : CRichEditCtrl(), def_text_color(RGB(0, 0, 0)) {}
    virtual ~MyRichEdit() {}

    void SetRichEditFont(bool bold, bool italic, const COLORREF* color = NULL, const LONG* txt_size = NULL);
    void AddText (const char * text, bool bold = TRUE, bool italic = FALSE, const COLORREF* color = NULL, const LONG* txt_size = NULL);
    void AddTextWithCaption(const char * caption, const char * value = NULL );
    void SetDefTextColor(const COLORREF& color) { def_text_color = color; }

    void AddTextW(const wchar_t * text, bool bold = TRUE, bool italic = FALSE, const COLORREF* color = NULL, const LONG* txt_size = NULL);
    void AddTextWithCaptionW(const wchar_t * caption, const wchar_t * value = NULL );
    void ResetContent() { SetWindowText(""); }

private:
    COLORREF def_text_color;
};
