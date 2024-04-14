//
// Created by 23303 on 2024/4/14.
//

#include "TrayWindow.h"
#include "pch.h"
#include "BorderlessWindow.hpp"

TrayWindow::TrayWindow(HWND parent,void * userdata) {
    parent=parent;

    auto window=static_cast<BorderlessWindow*>(userdata);
    // create trayicon
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = parent;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = window->hIcon;
    wcscpy_s(nid.szTip, L"Tray Icon Example");
    Shell_NotifyIcon(NIM_ADD, &nid);
}
