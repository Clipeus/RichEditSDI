#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN

// Windows Header Files
#include <windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <shlobj_core.h>
#include <MLang.h>
#include <comdef.h>
#include <Richedit.h>
#include <commdlg.h>

// C RunTime Header Files
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <variant>
#include <cassert>

#include <Misc.h>
#include <WinApp.h>
#include <Window.h>
#include <Dialog.h>
#include <MainWindow.h>
#include <Registry.h>
#include <OwnerDrawMenuImpl.h>
#include <ToolBar.h>
#include <WindowSubclass.h>
