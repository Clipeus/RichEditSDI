#include "WinApp.h"
#include "MainWindow.h"
#include <exception>

#pragma comment(lib, "comctl32.lib")

#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  try
  {
    Yaswl::WinApp app(L"Test");
    if (!app.Init(hInstance, lpCmdLine))
      return 0;

    Yaswl::MainWindow wnd;
    if (!wnd.Init() || !wnd.Create())
      return 0;

    ::ShowWindow(wnd, nCmdShow);
    return app.Run(&wnd);
  }
  catch (const std::exception& ex)
  {
    ::MessageBox(nullptr, Yaswl::a2w(ex.what()).c_str(), L"Unexception error", MB_OK | MB_ICONHAND);
  }
  return 0;
}
