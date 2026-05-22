#pragma once

class RichEditSDIWnd;

class RichEditSDIApp : public Yaswl::WinApp
{
public:
  RichEditSDIApp();
  ~RichEditSDIApp();

public:
  bool Init(HINSTANCE hInstance, const std::wstring& strCmdLine) override;
  int Run(Yaswl::Window* pMainWnd) override;
};
