#include "pch.h"
#include "RichEditSDI.h"
#include "RichEditSDIApp.h"
#include "RichEditSDIWnd.h"

using namespace Yaswl;

RichEditSDIApp::RichEditSDIApp()
{
  m_strAppName = LoadString(IDS_APP_TITLE);
  m_strRegistryRoot = L"SOFTWARE\\LMSoft\\RichEditSDI";
}

RichEditSDIApp::~RichEditSDIApp()
{
}

bool RichEditSDIApp::Init(HINSTANCE hInstance, const std::wstring& strCmdLine)
{
  HWND hWnd = ::FindWindow(RichEditSDIWnd::GetClassName(), nullptr);
  if (hWnd)
  {
    if (::IsIconic(hWnd))
      ::ShowWindow(hWnd, SW_RESTORE);
    ::SetForegroundWindow(hWnd);
    return false;
  }

  if (!WinApp::Init(hInstance, strCmdLine))
  {
    ReportSystemError(IDS_SYSTEM_ERROR);
    return false;
  }

  return true;
}

int RichEditSDIApp::Run(Yaswl::Window* pMainWnd)
{
  m_hAccel = ::LoadAccelerators(GetInstance(), MAKEINTRESOURCE(IDR_MAIN_ACCEL));
  if (!m_hAccel)
  {
    ReportSystemError(IDS_SYSTEM_ERROR);
    return 1;
  }

  return WinApp::Run(pMainWnd);
}
