#pragma once

class RichEditSDIWnd : public Yaswl::MainWindow, public Yaswl::OwnerDrawMenuImpl<RichEditSDIWnd>
{
  IMPLEMENT_ODW(RichEditSDIWnd, m_wndToolBar);

public:
  RichEditSDIWnd();
  ~RichEditSDIWnd();

public:
  bool Init();
  bool Create();

public:
  static const wchar_t* GetClassName();
  LRESULT SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);

protected:
  void OnRegisterClass(WNDCLASSEX& wc) override;

private:
  static DWORD CALLBACK EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb);
  static DWORD CALLBACK EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb);
  static bool IsRtfFile(const std::wstring& strPath);

  std::wstring SelectFile(bool bOpen, const std::wstring& strDefFileName = L"", int* pCodePage = nullptr);
  bool OpenFile(const std::wstring& strFileName);
  bool SaveFile(const std::wstring& strFileName, int nCodePage);
  void ChangeUIState(HMENU hContextMenu = nullptr);
  void UpdateStatusBarText();
  int ShowSaveConfirmDlg() const;

  void OnFormatBold();
  void OnFormatItalic();
  void OnFormatUnderline();
  void OnFormatFont();
  void OnFormatColor();

private:
  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
  void OnDestroy(HWND hWnd);
  bool OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
  void OnSize(HWND hWnd, UINT state, int cx, int cy);
  void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
  void OnMenuSelect(HWND hWnd, HMENU hMenu, int item, HMENU hMenuPopup, UINT flags);
  void OnContextMenu(HWND hWnd, HWND hwndContext, UINT x, UINT y);
  LRESULT OnNotify(HWND hWnd, int idFrom, NMHDR* pNmndr);
  bool OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
  void OnActivate(HWND hWnd, UINT nState, HWND hWndActDeact, bool fMinimized);
  void OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, bool fSystemMenu);

  bool OnOpenFile(bool bNew = false);
  bool OnSaveFile(bool bSaveAs = false);

private:
  bool m_bComInited = false;
  bool m_bDirty = false;
  HMODULE m_hRichEditLib = nullptr;
  COLORREF m_customColors[16] = {};
  Yaswl::WindowSubclassEx<RichEditSDIWnd> m_wndEditView;
  Yaswl::ToolBar m_wndToolBar;
  HWND m_hStatusBar = nullptr;
  std::wstring m_strFileName;
  int m_nCodePage = CP_UTF8;
};
