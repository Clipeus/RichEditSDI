#pragma once

class AboutDlg : public Yaswl::Dialog
{
public:
  AboutDlg();

private:
  bool DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  bool OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
};
