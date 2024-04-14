//
// Created by 23303 on 2024/4/14.
//

#ifndef BORDERLESSWINDOW_PCH_H
#define BORDERLESSWINDOW_PCH_H

#include <stdexcept>
#include <system_error>

#include <windowsx.h>
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "user32.lib")
#include <wrl.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <dcomp.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dcomp")

using namespace Microsoft::WRL;

#define ID_TRAY_ICON 1001
#define ID_TRAY_EXIT 1002

#define WM_TRAY_ICON (WM_USER + 1)

#endif //BORDERLESSWINDOW_PCH_H
