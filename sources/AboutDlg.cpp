#include "pch.h"
#include "RichEditSDI.h"
#include "AboutDlg.h"

AboutDlg::AboutDlg() : Dialog(MAKEINTRESOURCE(IDD_ABOUTBOX))
{
}

bool AboutDlg::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
  ::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)::LoadIcon(Yaswl::GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN)));
  ::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)::LoadIcon(Yaswl::GetApp()->GetInstance(), MAKEINTRESOURCE(IDI_MAIN)));
  return Dialog::OnInitDialog(hWnd, hWndFocus, lParam);
}

bool AboutDlg::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    HANDLE_DLGMSG(hWnd, WM_INITDIALOG, OnInitDialog);
  }
  return Dialog::DlgProc(hWnd, uMsg, wParam, lParam);
}
