#include "pch.h"
#include "RichEditSDI.h"
#include "RichEditSDIApp.h"
#include "RichEditSDIWnd.h"
#include "AboutDlg.h"
#include "TextEncoding.h"

constexpr int IDC_MAIN_STATUSBAR = 1;
constexpr int IDC_EDIT_VIEW      = IDC_MAIN_STATUSBAR + 1;
constexpr int IDC_MAIN_TOOLBAR   = IDC_EDIT_VIEW + 1;

constexpr int STATUSBAR_PART1_WIDTH = 100;
constexpr int STATUSBAR_PART2_WIDTH = 300;

using namespace Yaswl;

// ─── static helpers ──────────────────────────────────────────────────────────

DWORD CALLBACK RichEditSDIWnd::EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
  HANDLE hFile = reinterpret_cast<HANDLE>(dwCookie);
  return !::ReadFile(hFile, pbBuff, cb, reinterpret_cast<DWORD*>(pcb), nullptr);
}

DWORD CALLBACK RichEditSDIWnd::EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
  HANDLE hFile = reinterpret_cast<HANDLE>(dwCookie);
  return !::WriteFile(hFile, pbBuff, cb, reinterpret_cast<DWORD*>(pcb), nullptr);
}

bool RichEditSDIWnd::IsRtfFile(const std::wstring& strPath)
{
  std::filesystem::path p = strPath;
  return ::_wcsicmp(p.extension().c_str(), L".rtf") == 0;
}

// ─── construction ────────────────────────────────────────────────────────────

RichEditSDIWnd::RichEditSDIWnd()
  : MainWindow(RichEditSDIWnd::GetClassName())
  , m_wndEditView(this, &RichEditSDIWnd::SubWndProc)
{
}

RichEditSDIWnd::~RichEditSDIWnd()
{
}

void RichEditSDIWnd::OnRegisterClass(WNDCLASSEX& wc)
{
  wc.hIcon   = (HICON)LoadImage(GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, 0);
  wc.hIconSm = (HICON)LoadImage(GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 16, 16, 0);
  wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAIN);
}

bool RichEditSDIWnd::Init()
{
  if (!MainWindow::Init())
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }
  return true;
}

bool RichEditSDIWnd::Create()
{
  if (!WindowEx::Create(WS_EX_ACCEPTFILES, GetClassName(), GetApp()->GetAppName()))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }
  _ASSERTE(m_hWnd);
  ::UpdateWindow(m_hWnd);
  return true;
}

const wchar_t* RichEditSDIWnd::GetClassName()
{
  return L"RichEditSDIApp_MainWindow_1.0";
}

// ─── file dialogs ────────────────────────────────────────────────────────────

std::wstring RichEditSDIWnd::SelectFile(bool bOpen, const std::wstring& strDefFileName, int* pCodePage)
{
  WinApp::WaitCursor wc;
  std::wstring path;

  if (!m_bComInited)
  {
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    _ASSERTE(SUCCEEDED(hr));
    if (!SUCCEEDED(hr))
      return path;
    m_bComInited = true;
  }

  _COM_SMARTPTR_TYPEDEF(IFileDialog, __uuidof(IFileDialog));
  _COM_SMARTPTR_TYPEDEF(IFileDialogCustomize, __uuidof(IFileDialogCustomize));
  _COM_SMARTPTR_TYPEDEF(IShellItem, __uuidof(IShellItem));

  _com_ptr_t<IFileDialogPtr> pfd;
  if (SUCCEEDED(pfd.CreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER)))
  {
    constexpr int CTRL_ID = 1;
    _com_ptr_t<IFileDialogCustomizePtr> pfdc = pfd;

    // Encoding combo only for Save-as-text; ignored for RTF
    if (!bOpen)
    {
      pfdc->AddComboBox(CTRL_ID);
      pfdc->AddControlItem(CTRL_ID, TextEncoding::GetOEMCodePage(), GetApp()->LoadString(IDS_OEM_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, TextEncoding::GetANSICodePage(), GetApp()->LoadString(IDS_ANSI_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, CP_UTF7,  GetApp()->LoadString(IDS_UTF7_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, CP_UTF8,  GetApp()->LoadString(IDS_UTF8_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, 1200, GetApp()->LoadString(IDS_UTF16_LE_TXT).c_str());
      pfdc->AddControlItem(CTRL_ID, 1201, GetApp()->LoadString(IDS_UTF16_BE_TXT).c_str());

      if (pCodePage)
      {
        if (S_OK != pfdc->SetSelectedControlItem(CTRL_ID, *pCodePage))
        {
          CPINFOEX cpInfo = { 0 };
          ::GetCPInfoEx(*pCodePage, CP_ACP, &cpInfo);
          pfdc->AddControlItem(CTRL_ID, *pCodePage, cpInfo.CodePageName);
          pfdc->SetSelectedControlItem(CTRL_ID, *pCodePage);
        }
      }
    }

    DWORD dwOptions;
    if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
    {
      if (bOpen)
        pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
      else
        pfd->SetOptions(dwOptions | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN);
    }

    // Filter: Text, RTF, All
    COMDLG_FILTERSPEC filter[3];
    std::wstring str1 = GetApp()->LoadString(IDS_FILE_FILTER_TXT);
    filter[0].pszName = str1.c_str();
    std::wstring str2 = GetApp()->LoadString(IDS_FILE_FILTER_TXT_EXT);
    filter[0].pszSpec = str2.c_str();
    std::wstring str3 = GetApp()->LoadString(IDS_FILE_FILTER_RTF);
    filter[1].pszName = str3.c_str();
    std::wstring str4 = GetApp()->LoadString(IDS_FILE_FILTER_RTF_EXT);
    filter[1].pszSpec = str4.c_str();
    std::wstring str5 = GetApp()->LoadString(IDS_FILE_FILTER_ALL);
    filter[2].pszName = str5.c_str();
    std::wstring str6 = GetApp()->LoadString(IDS_FILE_FILTER_ALL_EXT);
    filter[2].pszSpec = str6.c_str();
    pfd->SetFileTypes(3, filter);

    pfd->SetDefaultExtension(str2.substr(2).c_str()); // default = .txt
    if (strDefFileName.size())
    {
      std::filesystem::path defPath = strDefFileName;
      pfd->SetFileName(defPath.filename().wstring().c_str());
    }

    if (SUCCEEDED(pfd->Show(m_hWnd)))
    {
      _com_ptr_t<IShellItemPtr> psi;
      if (SUCCEEDED(pfd->GetResult(&psi)))
      {
        LPWSTR lpFolder;
        if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &lpFolder)))
        {
          path = lpFolder;
          ::CoTaskMemFree(lpFolder);

          if (!bOpen && pCodePage)
            pfdc->GetSelectedControlItem(CTRL_ID, reinterpret_cast<DWORD*>(pCodePage));
        }
      }
    }
  }

  return path;
}

// ─── file I/O ────────────────────────────────────────────────────────────────

bool RichEditSDIWnd::OpenFile(const std::wstring& strFileName)
{
  Yaswl::unique_handle hFile = Yaswl::make_unique_handle(
    ::CreateFile(strFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (hFile.get() == INVALID_HANDLE_VALUE)
  {
    GetApp()->ReportSystemError(IDS_OPEN_FILE_FAILED);
    return false;
  }

  if (IsRtfFile(strFileName))
  {
    EDITSTREAM es = {};
    es.dwCookie   = reinterpret_cast<DWORD_PTR>(hFile.get());
    es.pfnCallback = EditStreamInCallback;
    ::SendMessage(m_wndEditView, EM_STREAMIN, SF_RTF, reinterpret_cast<LPARAM>(&es));
    return es.dwError == 0;
  }

  // Plain text: read bytes, detect encoding, convert to Unicode, push via EM_SETTEXTEX
  LARGE_INTEGER li;
  if (!::GetFileSizeEx(hFile.get(), &li))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }
  if (li.HighPart > 0)
  {
    GetApp()->ReportBox(IDS_FILE_IS_TO_LARGE, MB_OK | MB_ICONERROR);
    return false;
  }

  std::vector<uint8_t> vecBuffer(li.LowPart);
  DWORD dwRead = 0;
  if (!::ReadFile(hFile.get(), vecBuffer.data(), static_cast<DWORD>(vecBuffer.size()), &dwRead, nullptr))
  {
    GetApp()->ReportSystemError(IDS_READ_FILE_FAILED);
    return false;
  }

  TextEncoding te(vecBuffer.data(), vecBuffer.size());
  m_nCodePage = te.DetectCodePage();
  size_t nDestSize = te.GetToUnicodeCharCount(m_nCodePage);
  if (nDestSize == static_cast<size_t>(-1))
    return false;

  std::wstring wstrText(nDestSize, L'\0');
  if (!te.ToUnicode(&wstrText[0], nDestSize, m_nCodePage))
    return false;

  SETTEXTEX stex = {};
  stex.flags    = ST_DEFAULT;
  stex.codepage = 1200; // Unicode
  ::SendMessage(m_wndEditView, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stex),
                reinterpret_cast<LPARAM>(wstrText.c_str()));
  return true;
}

bool RichEditSDIWnd::SaveFile(const std::wstring& strFileName, int nCodePage)
{
  Yaswl::unique_handle hFile = Yaswl::make_unique_handle(
    ::CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (hFile.get() == INVALID_HANDLE_VALUE)
  {
    GetApp()->ReportSystemError(IDS_SAVE_FILE_FAILED);
    return false;
  }

  if (IsRtfFile(strFileName))
  {
    EDITSTREAM es = {};
    es.dwCookie    = reinterpret_cast<DWORD_PTR>(hFile.get());
    es.pfnCallback = EditStreamOutCallback;
    ::SendMessage(m_wndEditView, EM_STREAMOUT, SF_RTF, reinterpret_cast<LPARAM>(&es));
    return es.dwError == 0;
  }

  // Plain text: get Unicode content, convert, write
  GETTEXTLENGTHEX gtlex = {};
  gtlex.flags    = GTL_DEFAULT | GTL_PRECISE;
  gtlex.codepage = 1200;
  LRESULT nLength = ::SendMessage(m_wndEditView, EM_GETTEXTLENGTHEX,
                                  reinterpret_cast<WPARAM>(&gtlex), 0);
  if (nLength <= 0)
    return true; // empty document — write nothing

  std::wstring wstrText(static_cast<size_t>(nLength), L'\0');
  GETTEXTEX gtex = {};
  gtex.cb       = static_cast<DWORD>((nLength + 1) * sizeof(wchar_t));
  gtex.flags    = GT_DEFAULT;
  gtex.codepage = 1200;
  ::SendMessage(m_wndEditView, EM_GETTEXTEX,
                reinterpret_cast<WPARAM>(&gtex),
                reinterpret_cast<LPARAM>(wstrText.data()));

  TextEncoding te(wstrText.data(), static_cast<size_t>(nLength));
  size_t nDestSize = te.GetFromUnicodeBufferSize(nCodePage);
  if (nDestSize == static_cast<size_t>(-1))
    return false;

  std::vector<char> vecBuffer(nDestSize);
  te.FromUnicode(nCodePage, vecBuffer.data(), vecBuffer.size());

  DWORD dwWritten = 0;
  if (!::WriteFile(hFile.get(), vecBuffer.data(), static_cast<DWORD>(vecBuffer.size()), &dwWritten, nullptr))
  {
    GetApp()->ReportSystemError(IDS_WRITE_FILE_FAILED);
    return false;
  }
  return true;
}

// ─── UI state ────────────────────────────────────────────────────────────────

void RichEditSDIWnd::ChangeUIState(HMENU hContextMenu)
{
  CHARRANGE cr = {};
  ::SendMessage(m_wndEditView, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
  bool hasSelection = (cr.cpMin != cr.cpMax);

  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_CUT,    MAKELONG(hasSelection, 0));
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_COPY,   MAKELONG(hasSelection, 0));
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_DELETE, MAKELONG(hasSelection, 0));
  ::SendMessage(m_wndToolBar, TB_ENABLEBUTTON, ID_EDIT_UNDO,
                MAKELONG(::SendMessage(m_wndEditView, EM_CANUNDO, 0, 0) != 0, 0));

  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
  ::SendMessage(m_wndToolBar, TB_CHECKBUTTON, ID_FORMAT_BOLD,      MAKELONG((cf.dwEffects & CFE_BOLD)      != 0, 0));
  ::SendMessage(m_wndToolBar, TB_CHECKBUTTON, ID_FORMAT_ITALIC,    MAKELONG((cf.dwEffects & CFE_ITALIC)    != 0, 0));
  ::SendMessage(m_wndToolBar, TB_CHECKBUTTON, ID_FORMAT_UNDERLINE, MAKELONG((cf.dwEffects & CFE_UNDERLINE) != 0, 0));

  std::filesystem::path filePath = m_strFileName;
  std::wstring fname = filePath.filename().wstring();

  BOOL bModified = static_cast<BOOL>(::SendMessage(m_wndEditView, EM_GETMODIFY, 0, 0));
  std::wstring strTitle = bModified ? L"*" : L"";
  strTitle += (fname.size() ? fname : GetApp()->LoadString(IDS_UNTITLED_TXT));
  strTitle += L" - " + GetApp()->LoadString(IDS_APP_TITLE);
  ::SetWindowText(m_hWnd, strTitle.c_str());

  UpdateStatusBarText();
}

void RichEditSDIWnd::UpdateStatusBarText()
{
  ::SendMessage(m_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(GetApp()->LoadString(IDS_READY).c_str()));

  // Encoding section (only meaningful for text files)
  std::wstring strCodePage = GetApp()->LoadString(ID_CODEPAGE_TXT);
  if (IsRtfFile(m_strFileName))
  {
    strCodePage += L"RTF";
  }
  else
  {
    CPINFOEX cpInfo = { 0 };
    if (::GetCPInfoEx(m_nCodePage, CP_ACP, &cpInfo))
      strCodePage += cpInfo.CodePageName;
    else if (m_nCodePage == 1200)
      strCodePage += GetApp()->LoadString(IDS_UTF16_LE_TXT);
    else if (m_nCodePage == 1201)
      strCodePage += GetApp()->LoadString(IDS_UTF16_BE_TXT);
    else
      strCodePage += GetApp()->LoadString(IDS_UNICODE_TXT);
  }

  CHARRANGE cr = {};
  ::SendMessage(m_wndEditView, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
  int nLine     = static_cast<int>(::SendMessage(m_wndEditView, EM_LINEFROMCHAR, cr.cpMin, 0)) + 1;
  int nLineStart = static_cast<int>(::SendMessage(m_wndEditView, EM_LINEINDEX, nLine - 1, 0));
  int nCol      = cr.cpMin - nLineStart + 1;

  ::SendMessage(m_hStatusBar, SB_SETTEXT, 1,
                reinterpret_cast<LPARAM>(StrFormat(GetApp()->LoadString(IDS_LINE_COL_TXT).c_str(), nLine, nCol).c_str()));
  ::SendMessage(m_hStatusBar, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(strCodePage.c_str()));
}

int RichEditSDIWnd::ShowSaveConfirmDlg() const
{
  std::wstring strText = StrFormat(GetApp()->LoadString(IDS_SAVE_TO_FILE_TXT).c_str(),
    m_strFileName.size() ? m_strFileName.c_str() : GetApp()->LoadString(IDS_UNTITLED_TXT).c_str());

  TASKDIALOG_BUTTON tdb[2] = { 0 };
  tdb[0].nButtonID  = IDYES;
  tdb[0].pszButtonText = MAKEINTRESOURCE(IDS_SAVE_BTN_TXT);
  tdb[1].nButtonID  = IDNO;
  tdb[1].pszButtonText = MAKEINTRESOURCE(IDS_NO_SAVE_BTN_TXT);

  TASKDIALOGCONFIG tdc = { 0 };
  tdc.cbSize          = sizeof(tdc);
  tdc.hwndParent      = m_hWnd;
  tdc.hInstance       = GetApp()->GetInstance();
  tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
  tdc.pszWindowTitle  = MAKEINTRESOURCE(IDS_APP_TITLE);
  tdc.pszMainInstruction = strText.c_str();
  tdc.cButtons        = 2;
  tdc.pButtons        = tdb;

  int nButton = -1;
  if (S_OK != ::TaskDialogIndirect(&tdc, &nButton, nullptr, nullptr))
    return -1;
  return nButton;
}

// ─── formatting ──────────────────────────────────────────────────────────────

void RichEditSDIWnd::OnFormatBold()
{
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_BOLD;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
  cf.dwEffects ^= CFE_BOLD;
  ::SendMessage(m_wndEditView, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
}

void RichEditSDIWnd::OnFormatItalic()
{
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_ITALIC;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
  cf.dwEffects ^= CFE_ITALIC;
  ::SendMessage(m_wndEditView, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
}

void RichEditSDIWnd::OnFormatUnderline()
{
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_UNDERLINE;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
  cf.dwEffects ^= CFE_UNDERLINE;
  ::SendMessage(m_wndEditView, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
}

void RichEditSDIWnd::OnFormatFont()
{
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_ALL2;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

  LOGFONT lf = {};
  // yHeight is in twips (1/20pt); convert to lfHeight pixels
  if (cf.yHeight > 0)
  {
    HDC hDC = ::GetDC(m_wndEditView);
    lf.lfHeight = -::MulDiv(cf.yHeight, ::GetDeviceCaps(hDC, LOGPIXELSY), 1440);
    ::ReleaseDC(m_wndEditView, hDC);
  }
  lf.lfWeight        = (cf.dwEffects & CFE_BOLD)      ? FW_BOLD   : FW_NORMAL;
  lf.lfItalic        = (cf.dwEffects & CFE_ITALIC)    ? TRUE      : FALSE;
  lf.lfUnderline     = (cf.dwEffects & CFE_UNDERLINE) ? TRUE      : FALSE;
  lf.lfStrikeOut     = (cf.dwEffects & CFE_STRIKEOUT) ? TRUE      : FALSE;
  lf.lfCharSet       = cf.bCharSet;
  lf.lfPitchAndFamily = cf.bPitchAndFamily;
  ::wcscpy_s(lf.lfFaceName, cf.szFaceName);

  CHOOSEFONT chf = {};
  chf.lStructSize = sizeof(chf);
  chf.hwndOwner   = m_hWnd;
  chf.lpLogFont   = &lf;
  chf.rgbColors   = (cf.dwEffects & CFE_AUTOCOLOR) ? RGB(0, 0, 0) : cf.crTextColor;
  chf.Flags       = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;

  if (!::ChooseFont(&chf))
    return;

  CHARFORMAT2 newCf = {};
  newCf.cbSize  = sizeof(newCf);
  newCf.dwMask  = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR | CFM_CHARSET;
  newCf.yHeight = chf.iPointSize * 2; // iPointSize is tenths of a point; yHeight is twips (1/20pt)
  newCf.dwEffects = 0;
  if (lf.lfWeight >= FW_BOLD) newCf.dwEffects |= CFE_BOLD;
  if (lf.lfItalic)            newCf.dwEffects |= CFE_ITALIC;
  if (lf.lfUnderline)         newCf.dwEffects |= CFE_UNDERLINE;
  if (lf.lfStrikeOut)         newCf.dwEffects |= CFE_STRIKEOUT;
  newCf.crTextColor    = chf.rgbColors;
  newCf.bCharSet       = lf.lfCharSet;
  newCf.bPitchAndFamily = lf.lfPitchAndFamily;
  ::wcscpy_s(newCf.szFaceName, lf.lfFaceName);

  ::SendMessage(m_wndEditView, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&newCf));
}

void RichEditSDIWnd::OnFormatColor()
{
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_COLOR;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

  CHOOSECOLOR cc = {};
  cc.lStructSize  = sizeof(cc);
  cc.hwndOwner    = m_hWnd;
  cc.rgbResult    = (cf.dwEffects & CFE_AUTOCOLOR) ? RGB(0, 0, 0) : cf.crTextColor;
  cc.lpCustColors = m_customColors;
  cc.Flags        = CC_FULLOPEN | CC_RGBINIT;

  if (!::ChooseColor(&cc))
    return;

  CHARFORMAT2 newCf = {};
  newCf.cbSize      = sizeof(newCf);
  newCf.dwMask      = CFM_COLOR;
  newCf.crTextColor = cc.rgbResult;
  ::SendMessage(m_wndEditView, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&newCf));
}

// ─── subclass proc ───────────────────────────────────────────────────────────

LRESULT RichEditSDIWnd::SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
  if (hWnd == m_wndEditView)
  {
    switch (uMsg)
    {
      HANDLE_MSG(hWnd, WM_CONTEXTMENU, OnContextMenu), bHandled = true;
    }
  }
  return 0;
}

// ─── window procedure ────────────────────────────────────────────────────────

LRESULT RichEditSDIWnd::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    HANDLE_MSG(hWnd, WM_CREATE,       OnCreate);
    HANDLE_MSG(hWnd, WM_SIZE,         OnSize);
    HANDLE_MSG(hWnd, WM_DESTROY,      OnDestroy);
    HANDLE_MSG(hWnd, WM_MENUSELECT,   OnMenuSelect);
    HANDLE_MSG(hWnd, WM_COMMAND,      OnCommand);
    HANDLE_MSG(hWnd, WM_NOTIFY,       OnNotify);
    HANDLE_MSG(hWnd, WM_SETCURSOR,    OnSetCursor);
    HANDLE_MSG(hWnd, WM_ACTIVATE,     OnActivate);
    HANDLE_MSG(hWnd, WM_INITMENUPOPUP, OnInitMenuPopup);
    HANDLE_MSG(hWnd, WM_MEASUREITEM,  OnMeasureItem), true;
    HANDLE_MSG(hWnd, WM_DRAWITEM,     OnDrawItem), true;
    default:
      return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}

bool RichEditSDIWnd::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
  m_hWnd = hWnd;

  m_hRichEditLib = ::LoadLibrary(L"Msftedit.dll");
  if (!m_hRichEditLib)
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  TBBUTTON tbButtons[] =
  {
    {IDI_NEW,    ID_FILE_NEW,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_OPEN,   ID_FILE_OPEN,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_SAVE,   ID_FILE_SAVE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_UNDO,   ID_EDIT_UNDO,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_CUT,    ID_EDIT_CUT,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_COPY,   ID_EDIT_COPY,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_PASTE,  ID_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_DELETE,    ID_EDIT_DELETE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_BOLD,      ID_FORMAT_BOLD,      TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0},
    {IDI_ITALIC,    ID_FORMAT_ITALIC,    TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0},
    {IDI_UNDERLINE, ID_FORMAT_UNDERLINE, TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_FONT,      ID_FORMAT_FONT,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {IDI_COLOR,     ID_FORMAT_COLOR,     TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
    {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0},
    {IDI_HELP,      IDM_ABOUT,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
  };

  if (!m_wndToolBar.Create(m_hWnd, IDC_MAIN_TOOLBAR))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }
  if (!m_wndToolBar.Init(tbButtons, sizeof(tbButtons) / sizeof(tbButtons[0])))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  m_hStatusBar = ::CreateWindowEx(0, STATUSCLASSNAME, nullptr,
    WS_CHILD | WS_VISIBLE | WS_BORDER | SBARS_SIZEGRIP,
    0, 0, 0, 0, m_hWnd,
    reinterpret_cast<HMENU>(static_cast<long long>(IDC_MAIN_STATUSBAR)),
    GetApp()->GetInstance(), nullptr);
  if (!m_hStatusBar)
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  RECT rect;
  ::GetClientRect(m_hWnd, &rect);

  if (!m_wndEditView.Create(0, MSFTEDIT_CLASS, L"",
    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
    ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
    0, 0, rect.right, rect.bottom, m_hWnd,
    reinterpret_cast<HMENU>(static_cast<long long>(IDC_EDIT_VIEW)),
    GetApp()->GetInstance()))
  {
    GetApp()->ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  // No hard limit; use 2 GB
  ::SendMessage(m_wndEditView, EM_EXLIMITTEXT, 0, 0x7FFFFFFF);

  // Request selection-change and text-change notifications
  ::SendMessage(m_wndEditView, EM_SETEVENTMASK, 0, ENM_SELCHANGE | ENM_CHANGE);

  HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
  LOGFONT lf;
  ::GetObject(hFont, sizeof(LOGFONT), &lf);

  HDC hDC = ::GetDC(m_wndEditView);
  lf.lfHeight = -::MulDiv(11, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
  ::ReleaseDC(m_wndEditView, hDC);

  hFont = ::CreateFontIndirect(&lf);
  ::SendMessage(m_wndEditView, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 0);

  m_wndEditView.Subclass(true);

  ChangeUIState();
  return true;
}

void RichEditSDIWnd::OnDestroy(HWND hWnd)
{
  SaveWindowPlacement();

  if (m_bComInited)
    ::CoUninitialize();

  HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(m_wndEditView, WM_GETFONT, 0, 0));
  if (hFont)
    ::DeleteObject(hFont);

  if (m_hRichEditLib)
    ::FreeLibrary(m_hRichEditLib);

  ::PostQuitMessage(0);
}

void RichEditSDIWnd::OnSize(HWND hWnd, UINT state, int cx, int cy)
{
  ::SendMessage(m_wndToolBar, WM_SIZE, state, 0);
  ::SendMessage(m_hStatusBar, WM_SIZE, state, 0);

  int ptWidth[3];
  ptWidth[0] = cx - (STATUSBAR_PART1_WIDTH + STATUSBAR_PART2_WIDTH);
  ptWidth[1] = cx - STATUSBAR_PART2_WIDTH;
  ptWidth[2] = -1;
  ::SendMessage(m_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(ptWidth));

  RECT rectTB, rectSB;
  ::GetWindowRect(m_wndToolBar, &rectTB);
  int nTop    = rectTB.bottom - rectTB.top - 1;
  ::GetWindowRect(m_hStatusBar, &rectSB);
  int nBottom = rectSB.bottom - rectSB.top - 1;

  ::MoveWindow(m_wndEditView, 0, nTop, cx, cy - nBottom - nTop, true);
}

void RichEditSDIWnd::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  switch (id)
  {
    case IDC_EDIT_VIEW:
    {
      if (codeNotify == EN_MAXTEXT)
        GetApp()->ReportBox(IDS_MAXTEXT_TXT, MB_OK | MB_ICONINFORMATION);
      break;
    }
    case IDM_EXIT:
      ::PostMessage(hWnd, WM_CLOSE, 0, 0);
      break;

    case ID_FILE_NEW:
    case ID_FILE_OPEN:
      OnOpenFile(id == ID_FILE_NEW);
      ChangeUIState();
      break;

    case ID_FILE_SAVE:
    case ID_FILE_SAVEAS:
      OnSaveFile(id == ID_FILE_SAVEAS);
      ChangeUIState();
      break;

    case ID_EDIT_UNDO:
      ::SendMessage(m_wndEditView, EM_UNDO, 0, 0);
      ChangeUIState();
      break;

    case ID_EDIT_CUT:
      ::SendMessage(m_wndEditView, WM_CUT, 0, 0);
      ChangeUIState();
      break;

    case ID_EDIT_COPY:
      ::SendMessage(m_wndEditView, WM_COPY, 0, 0);
      ChangeUIState();
      break;

    case ID_EDIT_PASTE:
      ::SendMessage(m_wndEditView, WM_PASTE, 0, 0);
      ChangeUIState();
      break;

    case ID_EDIT_DELETE:
      ::SendMessage(m_wndEditView, WM_CLEAR, 0, 0);
      ChangeUIState();
      break;

    case ID_EDIT_SELECTALL:
    {
      CHARRANGE cr = { 0, -1 };
      ::SendMessage(m_wndEditView, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
      ChangeUIState();
      break;
    }

    case ID_FORMAT_BOLD:      OnFormatBold();      break;
    case ID_FORMAT_ITALIC:    OnFormatItalic();    break;
    case ID_FORMAT_UNDERLINE: OnFormatUnderline(); break;
    case ID_FORMAT_FONT:      OnFormatFont();      break;
    case ID_FORMAT_COLOR:     OnFormatColor();     break;

    case IDM_ABOUT:
    {
      AboutDlg dlg;
      dlg.DoModal(m_hWnd);
      break;
    }
  }
}

bool RichEditSDIWnd::OnOpenFile(bool bNew)
{
  if (::SendMessage(m_wndEditView, EM_GETMODIFY, 0, 0))
  {
    int nDlgResult = ShowSaveConfirmDlg();
    if (nDlgResult == IDYES)
    {
      if (!OnSaveFile())
        return false;
    }
    else if (nDlgResult == IDCANCEL)
    {
      return false;
    }
  }

  if (!bNew)
  {
    std::wstring path = SelectFile(true);
    if (!path.size())
      return false;

    if (OpenFile(path))
    {
      m_strFileName = path;
      if (IsRtfFile(path))
        m_nCodePage = CP_UTF8; // not used for RTF, but keep a sane default
    }
  }
  else
  {
    SETTEXTEX stex = {};
    stex.flags    = ST_DEFAULT;
    stex.codepage = 1200;
    ::SendMessage(m_wndEditView, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stex),
                  reinterpret_cast<LPARAM>(L""));
    m_strFileName.clear();
    m_nCodePage = CP_UTF8;
  }

  ::SendMessage(m_wndEditView, EM_SETMODIFY, FALSE, 0);
  ::SendMessage(m_wndEditView, EM_EMPTYUNDOBUFFER, 0, 0);
  return true;
}

bool RichEditSDIWnd::OnSaveFile(bool bSaveAs)
{
  std::wstring path = (!bSaveAs ? m_strFileName : L"");
  int nCodePage = m_nCodePage;

  if (!path.size())
    path = SelectFile(false, m_strFileName, &nCodePage);

  if (path.size())
  {
    if (SaveFile(path, nCodePage))
    {
      m_strFileName = path;
      if (!IsRtfFile(path))
        m_nCodePage = nCodePage;
      ::SendMessage(m_wndEditView, EM_SETMODIFY, FALSE, 0);
      return true;
    }
  }
  return false;
}

void RichEditSDIWnd::OnMenuSelect(HWND hWnd, HMENU hMenu, int item, HMENU hMenuPopup, UINT flags)
{
  if ((flags == 0xFFFFFFFF) && (hMenu == nullptr))
  {
    UpdateStatusBarText();
  }
  else
  {
    std::wstring strText;
    if (!((flags & MF_POPUP) || (flags & MFT_SEPARATOR)))
    {
      strText = GetApp()->LoadString(item);
      size_t pos = strText.find(L'\n');
      if (std::wstring::npos != pos)
        strText = strText.substr(0, pos);
    }
    ::SendMessage(m_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(strText.c_str()));
  }
}

void RichEditSDIWnd::OnContextMenu(HWND hWnd, HWND hwndContext, UINT x, UINT y)
{
  POINT pt;
  if (x == static_cast<UINT>(-1) || y == static_cast<UINT>(-1))
  {
    RECT rect;
    ::GetClientRect(m_wndEditView, &rect);
    pt.x = rect.right / 2;
    pt.y = rect.bottom / 2;
    ::ClientToScreen(m_wndEditView, &pt);
  }
  else
  {
    pt.x = static_cast<int>(x);
    pt.y = static_cast<int>(y);
    if (m_wndEditView != ::WindowFromPoint(pt))
      return;
  }

  HMENU hMenuMain    = ::LoadMenu(GetApp()->GetInstance(), MAKEINTRESOURCE(IDR_EDIT_CONTEXT_MENU));
  HMENU hMenuContext = ::GetSubMenu(hMenuMain, 0);
  ::TrackPopupMenu(hMenuContext, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
                   pt.x, pt.y, 0, m_hWnd, nullptr);
  ::DestroyMenu(hMenuContext);
  ::DestroyMenu(hMenuMain);
}

LRESULT RichEditSDIWnd::OnNotify(HWND hWnd, int idFrom, NMHDR* pNmndr)
{
  switch (pNmndr->code)
  {
    case EN_SELCHANGE:
      if (pNmndr->hwndFrom == m_wndEditView.GetHandle())
        ChangeUIState();
      return 0;

    case TTN_GETDISPINFO:
    {
      LPTOOLTIPTEXT lpTTT = reinterpret_cast<LPTOOLTIPTEXT>(pNmndr);
      std::wstring strText = GetApp()->LoadString(idFrom);
      size_t pos = strText.find(L'\n');
      if (std::wstring::npos != pos)
        strText = strText.substr(pos + 1);
      ::wcsncpy_s(lpTTT->szText, strText.c_str(), sizeof(lpTTT->szText));
      return true;
    }
  }
  return false;
}

bool RichEditSDIWnd::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
  if (GetApp()->IsWaitCursor())
  {
    ::SetCursor(GetApp()->GetWaitCursor());
    return true;
  }
  return FORWARD_WM_SETCURSOR(hWnd, hWndCursor, codeHitTest, msg, ::DefWindowProc);
}

void RichEditSDIWnd::OnActivate(HWND hWnd, UINT nState, HWND hWndActDeact, bool fMinimized)
{
  if (nState == WA_ACTIVE)
    ::SetFocus(m_wndEditView);
}

void RichEditSDIWnd::OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, bool fSystemMenu)
{
  if (fSystemMenu)
    return;

  // Get current char format for Bold/Italic/Underline check-states
  CHARFORMAT2 cf = {};
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
  ::SendMessage(m_wndEditView, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

  CHARRANGE cr = {};
  ::SendMessage(m_wndEditView, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
  bool hasSelection = (cr.cpMin != cr.cpMax);
  bool canUndo      = ::SendMessage(m_wndEditView, EM_CANUNDO, 0, 0) != 0;

  int nCount = ::GetMenuItemCount(hMenu);
  for (int i = 0; i < nCount; i++)
  {
    MENUITEMINFO mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_ID;
    ::GetMenuItemInfo(hMenu, i, true, &mii);

    switch (mii.wID)
    {
      case ID_FILE_SAVEAS:
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (m_strFileName.size() ? MFS_ENABLED : MFS_DISABLED));
        break;

      case ID_EDIT_UNDO:
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (canUndo ? MFS_ENABLED : MFS_DISABLED));
        break;

      case ID_EDIT_CUT:
      case ID_EDIT_COPY:
      case ID_EDIT_DELETE:
        ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND | (hasSelection ? MFS_ENABLED : MFS_DISABLED));
        break;

      case ID_FORMAT_BOLD:
        ::CheckMenuItem(hMenu, mii.wID, MF_BYCOMMAND | ((cf.dwEffects & CFE_BOLD)      ? MF_CHECKED : MF_UNCHECKED));
        break;

      case ID_FORMAT_ITALIC:
        ::CheckMenuItem(hMenu, mii.wID, MF_BYCOMMAND | ((cf.dwEffects & CFE_ITALIC)    ? MF_CHECKED : MF_UNCHECKED));
        break;

      case ID_FORMAT_UNDERLINE:
        ::CheckMenuItem(hMenu, mii.wID, MF_BYCOMMAND | ((cf.dwEffects & CFE_UNDERLINE) ? MF_CHECKED : MF_UNCHECKED));
        break;
    }
  }

  Yaswl::OwnerDrawMenuImpl<RichEditSDIWnd>::OnInitMenuPopup(hWnd, hMenu, item, fSystemMenu);
}
