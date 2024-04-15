#include "pch.h"

#include "BorderlessWindow.hpp"

int main() {
    try {
//        BorderlessWindow window;
        BorderlessWindow::RunApp();
    }
    catch (const std::exception &e) {
        ::MessageBoxA(nullptr, e.what(), "Unhandled Exception", MB_OK | MB_ICONERROR);
    }
}
//        MSG msg;
//        while (::GetMessageW(&msg, nullptr, 0, 0) == TRUE) {
//            ::TranslateMessage(&msg);
//            ::DispatchMessageW(&msg);
//        }
//    }
//    catch (const std::exception& e) {
//        ::MessageBoxA(nullptr, e.what(), "Unhandled Exception", MB_OK|MB_ICONERROR);
//    }
