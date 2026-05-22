#include "pch.h"
#include "RichEditSDI.h"
#include "RichEditSDIApp.h"
#include "RichEditSDIWnd.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  RichEditSDIApp app;
  if (!app.Init(hInstance, lpCmdLine))
    return 1;

  RichEditSDIWnd wnd;
  if (!wnd.Init() || !wnd.Create())
    return 1;

  wnd.RestoreWindowPlacement(nCmdShow);
  return app.Run(&wnd);
}
