//
// Created by 23303 on 2024/4/14.
//

#ifndef BORDERLESSWINDOW_TRAYWINDOW_H
#define BORDERLESSWINDOW_TRAYWINDOW_H

#include "pch.h"
class TrayWindow {
public:
    NOTIFYICONDATA nid;
    HWND hwnd= nullptr;
    HWND parent= nullptr;
    explicit TrayWindow(HWND parent,void* userdata);
    void showTrayWindowAt(LPPOINT point);
    static auto CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT;

    ~TrayWindow(){
        hwnd= nullptr;
        parent= nullptr;
    }
};


#endif //BORDERLESSWINDOW_TRAYWINDOW_H
