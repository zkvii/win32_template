#pragma once

#include "pch.h"
#include "TrayWindow.h"


class BorderlessWindow {
public:
    BorderlessWindow();
    auto set_borderless(bool enabled) -> void;
    auto set_borderless_shadow(bool enabled) -> void;
    HICON hIcon;

private:
    static auto CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT;
    auto hit_test(POINT cursor) const -> LRESULT;

    bool borderless        = true; // is the window currently borderless
    bool borderless_resize = true; // should the window allow resizing by dragging the borders while borderless
    bool borderless_drag   = true; // should the window allow moving my dragging the client area
    bool borderless_shadow = true; // should the window display a native aero shadow while borderless

    HWND handle;
    void set_opacity(float d);

private:
    ComPtr<ID3D11Device> direct3dDevice;
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIFactory2> dxFactory;
    ComPtr<IDXGISwapChain1> swapChain;
    ComPtr<ID2D1Factory2> d2Factory;
    ComPtr<ID2D1Device1> d2Device;
    ComPtr<ID2D1DeviceContext> dc;
    ComPtr<IDXGISurface2> surface;
    ComPtr<ID2D1Bitmap1> bitmap;
    ComPtr<IDCompositionDevice> dcompDevice;
    ComPtr<IDCompositionTarget> target;
    ComPtr<IDCompositionVisual> visual;
    ComPtr<ID2D1SolidColorBrush> brush;
    ComPtr<IDWriteFactory> writeFactory;


    void init_direct2d();

    void draw();

    void set_transparent_window(float d);

    TrayWindow* trayWindow= nullptr;

    void load_statics();
};
