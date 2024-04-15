//
// Created by 23303 on 2024/4/14.
//

#include "TrayWindow.h"
#include "pch.h"
#include "BorderlessWindow.hpp"

TrayWindow::TrayWindow(HWND parent, void *userdata) {
    parent = parent;

    auto window = static_cast<BorderlessWindow *>(userdata);
    // create trayicon
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = parent;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = window->hIcon;
    // tooltip string
    wcscpy_s(nid.szTip, L"TrayApp");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void TrayWindow::showTrayWindowAt(LPPOINT point) {
    // Register the window class if it's not already registered
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = &TrayWindow::WndProc;
    wc.hInstance = nullptr;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TrayWindowClass"; // Changed window class name
    if (!GetClassInfo(NULL, wc.lpszClassName, &wc)) {
        RegisterClass(&wc);
    }

    // Create the window
    hwnd = CreateWindowEx(
            0,
            wc.lpszClassName, // Use the registered window class name
            L"TrayWindow",
            WS_OVERLAPPEDWINDOW,
            point->x-100, point->y-400,
            200, 400,
            parent,
            nullptr,
            nullptr,
            nullptr);

    if (hwnd == NULL) {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
}

auto TrayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}
