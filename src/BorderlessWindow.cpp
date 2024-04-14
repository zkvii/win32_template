#include <iostream>
#include "pch.h"
#include "BorderLessWindow.hpp"


namespace {

    struct ComException {
        HRESULT result;

        ComException(HRESULT const value) :
                result(value) {}
    };

    void HR(HRESULT const result) {
        if (S_OK != result) {
            throw ComException(result);
        }
    }


    // we cannot just use WS_POPUP style
    // WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
    // WS_SYSMENU: enables the context menu with the move, close, maximize, minize... commands (shift + right-click on the task bar item)
    // WS_CAPTION: enables aero minimize animation/transition
    // WS_MAXIMIZEBOX, WS_MINIMIZEBOX: enable minimize/maximize
    enum class Style : DWORD {
        transparent = WS_EX_NOREDIRECTIONBITMAP,
        windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        basic_borderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
    };

    auto maximized(HWND hwnd) -> bool {
        WINDOWPLACEMENT placement;
        if (!::GetWindowPlacement(hwnd, &placement)) {
            return false;
        }

        return placement.showCmd == SW_MAXIMIZE;
    }

    /* Adjust client rect to not spill over monitor edges when maximized.
     * rect(in/out): in: proposed window rect, out: calculated client rect
     * Does nothing if the window is not maximized.
     */
    auto adjust_maximized_client_rect(HWND window, RECT &rect) -> void {
        if (!maximized(window)) {
            return;
        }

        auto monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
        if (!monitor) {
            return;
        }

        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        if (!::GetMonitorInfoW(monitor, &monitor_info)) {
            return;
        }

        // when maximized, make the client area fill just the monitor (without task bar) rect,
        // not the whole window rect which extends beyond the monitor.
        rect = monitor_info.rcWork;
    }

    auto last_error(const std::string &message) -> std::system_error {
        return std::system_error(
                std::error_code(::GetLastError(), std::system_category()),
                message
        );
    }

    auto window_class(WNDPROC wndproc,void * userdata) -> const wchar_t * {

        auto window=static_cast<BorderlessWindow*>(userdata);

        static const wchar_t *window_class_name = [&] {
            WNDCLASSEXW wcx{};
            wcx.cbSize = sizeof(wcx);
            wcx.style = CS_HREDRAW | CS_VREDRAW;
            wcx.hInstance = nullptr;
            wcx.lpfnWndProc = wndproc;
            wcx.lpszClassName = L"BorderlessWindowClass";
            wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);

            wcx.hIcon = window->hIcon;
            const ATOM result = ::RegisterClassExW(&wcx);
            if (!result) {
                throw last_error("failed to register window class");
            }
            return wcx.lpszClassName;
        }();
        return window_class_name;
    }

    auto composition_enabled() -> bool {
        BOOL composition_enabled = FALSE;
        bool success = ::DwmIsCompositionEnabled(&composition_enabled) == S_OK;
        return composition_enabled && success;
    }

    auto select_borderless_style() -> Style {
        return composition_enabled() ? Style::aero_borderless : Style::basic_borderless;
    }

    auto set_shadow(HWND handle, bool enabled) -> void {
        if (composition_enabled()) {
            static const MARGINS shadow_state[2]{{0, 0, 0, 0},
                                                 {1, 1, 1, 1}};
            ::DwmExtendFrameIntoClientArea(handle, &shadow_state[enabled]);
        }
    }

    auto create_window(WNDPROC wndproc, void *userdata) -> HWND {

        auto handle = CreateWindowExW(
                static_cast<DWORD>(Style::transparent), window_class(wndproc,userdata), L"Borderless Window",
                static_cast<DWORD>(Style::basic_borderless),
//                WS_EX_NOREDIRECTIONBITMAP,
                100,
                100,
                480, 400,
                nullptr, nullptr, nullptr,
                userdata
        );
        if (!handle) {
            throw last_error("failed to create window");
        }
        return handle;
    }
}

BorderlessWindow::BorderlessWindow(){
    load_statics();
    handle=create_window(&BorderlessWindow::WndProc, this);
    trayWindow=new TrayWindow(handle,this);
//    trayWindow = TrayWindow(handle);
//    set_borderless(borderless);
//    set_borderless_shadow(borderless_shadow);
    init_direct2d();
    ::ShowWindow(handle, SW_SHOW);
}

void BorderlessWindow::set_borderless(bool enabled) {
    Style new_style = (enabled) ? select_borderless_style() : Style::windowed;
    Style old_style = static_cast<Style>(::GetWindowLongPtrW(handle, GWL_STYLE));

    if (new_style != old_style) {
        borderless = enabled;

        ::SetWindowLongPtrW(handle, GWL_STYLE, static_cast<LONG>(new_style));

        // when switching between borderless and windowed, restore appropriate shadow state
        set_shadow(handle, borderless_shadow && (new_style != Style::windowed));

        // redraw frame
        ::SetWindowPos(handle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
        ::ShowWindow(handle, SW_SHOW);
    }
}

void BorderlessWindow::set_borderless_shadow(bool enabled) {
    if (borderless) {
        borderless_shadow = enabled;
        set_shadow(handle, enabled);
    }
}

auto CALLBACK BorderlessWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept -> LRESULT {
    if (msg == WM_NCCREATE) {
        auto userdata = reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams;
        // store window instance pointer in window user data
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
    }
    if (auto window_ptr = reinterpret_cast<BorderlessWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
        auto &window = *window_ptr;

        switch (msg) {
            case WM_TRAY_ICON: {
                switch (LOWORD(lparam)) {
                    case WM_RBUTTONDOWN: {
                        POINT cursor;
                        ::GetCursorPos(&cursor);
                        ::SetForegroundWindow(hwnd);
                        ::TrackPopupMenu(reinterpret_cast<HMENU>(wparam), 0, cursor.x, cursor.y, 0, hwnd, nullptr);
                        ::PostMessageW(hwnd, WM_NULL, 0, 0);
                        return 0;
                    }
                    case WM_LBUTTONDBLCLK: {
                        ::ShowWindow(hwnd, SW_RESTORE);
                        return 0;
                    }
                }
                break;
            }

            case WM_NCCALCSIZE: {
                if (wparam == TRUE && window.borderless) {
                    auto &params = *reinterpret_cast<NCCALCSIZE_PARAMS *>(lparam);
                    adjust_maximized_client_rect(hwnd, params.rgrc[0]);
                    return 0;
                }
                break;
            }
            case WM_NCHITTEST: {
                // When we have no border or title bar, we need to perform our
                // own hit testing to allow resizing and moving.
                if (window.borderless) {
                    return window.hit_test(POINT{
                            GET_X_LPARAM(lparam),
                            GET_Y_LPARAM(lparam)
                    });
                }
                break;
            }
            case WM_NCACTIVATE: {
                if (!composition_enabled()) {
                    // Prevents window frame reappearing on window activation
                    // in "basic" theme, where no aero shadow is present.
                    return 1;
                }
                break;
            }

            case WM_CLOSE: {
                ::DestroyWindow(hwnd);
                return 0;
            }

            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                switch (wparam) {
                    case VK_F8 : {
                        window.borderless_drag = !window.borderless_drag;
                        return 0;
                    }
                    case VK_F9 : {
                        window.borderless_resize = !window.borderless_resize;
                        return 0;
                    }
                    case VK_F10: {
                        window.set_borderless(!window.borderless);
                        return 0;
                    }
                    case VK_F11: {
                        window.set_borderless_shadow(!window.borderless_shadow);
                        return 0;
                    }
                    case VK_F7: {
                        window.set_opacity(0.5f);
                        return 0;
                    }
                    default:
                        break;
                }
                break;
            }
        }
    }

    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

auto BorderlessWindow::hit_test(POINT cursor) const -> LRESULT {
    // identify borders and corners to allow resizing the window.
    // Note: On Windows 10, windows behave differently and
    // allow resizing outside the visible window frame.
    // This implementation does not replicate that behavior.
    const POINT border{
            ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
            ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    RECT window;
    if (!::GetWindowRect(handle, &window)) {
        return HTNOWHERE;
    }

    const auto drag = borderless_drag ? HTCAPTION : HTCLIENT;

    enum region_mask {
        client = 0b0000,
        left = 0b0001,
        right = 0b0010,
        top = 0b0100,
        bottom = 0b1000,
    };

    const auto result =
            left * (cursor.x < (window.left + border.x)) |
            right * (cursor.x >= (window.right - border.x)) |
            top * (cursor.y < (window.top + border.y)) |
            bottom * (cursor.y >= (window.bottom - border.y));

    switch (result) {
        case left          :
            return borderless_resize ? HTLEFT : drag;
        case right         :
            return borderless_resize ? HTRIGHT : drag;
        case top           :
            return borderless_resize ? HTTOP : drag;
        case bottom        :
            return borderless_resize ? HTBOTTOM : drag;
        case top | left    :
            return borderless_resize ? HTTOPLEFT : drag;
        case top | right   :
            return borderless_resize ? HTTOPRIGHT : drag;
        case bottom | left :
            return borderless_resize ? HTBOTTOMLEFT : drag;
        case bottom | right:
            return borderless_resize ? HTBOTTOMRIGHT : drag;
        case client        :
            return drag;
        default            :
            return HTNOWHERE;
    }
}

void BorderlessWindow::set_opacity(float d) {
    std::cout << "set_opacity" << d << std::endl;
    set_transparent_window(d);

    draw();

}

void BorderlessWindow::init_direct2d() {
    HR(D3D11CreateDevice(nullptr,    // Adapter
                         D3D_DRIVER_TYPE_HARDWARE,
                         nullptr,    // Module
                         D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                         nullptr, 0, // Highest available feature level
                         D3D11_SDK_VERSION,
                         &direct3dDevice,
                         nullptr,    // Actual feature level
                         nullptr));  // Device context

    HR(direct3dDevice.As(&dxgiDevice));

    HR(CreateDXGIFactory2(
            DXGI_CREATE_FACTORY_DEBUG,
            __uuidof(dxFactory),
            reinterpret_cast<void **>(dxFactory.GetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 description = {};
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    description.BufferCount = 2;
    description.SampleDesc.Count = 1;
    description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    RECT rect = {};
    GetClientRect(handle, &rect);
    description.Width = rect.right - rect.left;
    description.Height = rect.bottom - rect.top;

    HR(dxFactory->CreateSwapChainForComposition(dxgiDevice.Get(),
                                                &description,
                                                nullptr, // Don’t restrict
                                                swapChain.GetAddressOf()));

    // Create a single-threaded Direct2D factory with debugging information
    D2D1_FACTORY_OPTIONS const options = {D2D1_DEBUG_LEVEL_INFORMATION};
    HR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                         options,
                         d2Factory.GetAddressOf()));
    // Create the Direct2D device that links back to the Direct3D device
    HR(d2Factory->CreateDevice(dxgiDevice.Get(),
                               d2Device.GetAddressOf()));
    // Create the Direct2D device context that is the actual render target
    // and exposes drawing commands
    HR(d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                     dc.GetAddressOf()));
    // Retrieve the swap chain's back buffer
    HR(swapChain->GetBuffer(
            0, // index
            __uuidof(surface),
            reinterpret_cast<void **>(surface.GetAddressOf())));
    // Create a Direct2D bitmap that points to the swap chain surface
    D2D1_BITMAP_PROPERTIES1 properties = {};
    properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET |
                               D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    HR(dc->CreateBitmapFromDxgiSurface(surface.Get(),
                                       properties,
                                       bitmap.GetAddressOf()));
    // Point the device context to the bitmap for rendering
    dc->SetTarget(bitmap.Get());

    HR(DCompositionCreateDevice(
            dxgiDevice.Get(),
            __uuidof(dcompDevice),
            reinterpret_cast<void **>(dcompDevice.GetAddressOf())));


    HR(dcompDevice->CreateTargetForHwnd(handle,
                                        true, // Top most
                                        target.GetAddressOf()));

    HR(dcompDevice->CreateVisual(visual.GetAddressOf()));
    HR(visual->SetContent(swapChain.Get()));
    HR(target->SetRoot(visual.Get()));
    HR(dcompDevice->Commit());

    D2D1_COLOR_F const brushColor = D2D1::ColorF(0.18f,  // red
                                                 0.55f,  // green
                                                 0.34f,  // blue
                                                 0.75f); // alpha
    HR(dc->CreateSolidColorBrush(brushColor,
                                 brush.GetAddressOf()));


    HR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                           __uuidof(writeFactory),
                           reinterpret_cast<IUnknown **>(writeFactory.GetAddressOf())));
}

void BorderlessWindow::draw() {
// Cycle through alpha values on each redraw
    static int i = -1;
    i++;
    if (i >= 3) i = 0;
    float alpha[] = {0.25f, 0.5f, 1.0f};

    // Draw something
    dc->BeginDraw();
    dc->Clear();

    D2D1_COLOR_F const brushColor = D2D1::ColorF(0.18f,  // red
                                                 0.55f,  // green
                                                 0.34f,  // blue
                                                 alpha[i]); // alpha
    brush->SetColor(brushColor);
    D2D1_POINT_2F const ellipseCenter = D2D1::Point2F(100.0f,  // x
                                                      100.0f); // y
    D2D1_ELLIPSE const ellipse = D2D1::Ellipse(ellipseCenter,
                                               100.0f,  // x radius
                                               100.0f); // y radius
    dc->FillEllipse(ellipse, brush.Get());

    std::wstring helloText = L"Hello, World!";
    D2D1_RECT_F textRect = D2D1::RectF(50.0f,  // left
                                       150.0f, // top
                                       150.0f, // right
                                       200.0f); // bottom
    brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));

    ComPtr<IDWriteTextFormat> textFormat;
    HR(writeFactory->CreateTextFormat(
            L"Arial", // font family
            nullptr,  // font collection
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            48.0f,    // font size
            L"en-us", // locale
            textFormat.GetAddressOf()
    ));

    dc->DrawText(
            helloText.c_str(),
            (UINT32) helloText.size(),
            textFormat.Get(),
            &textRect,
            brush.Get()
    );
    HR(dc->EndDraw());

    // Make the swap chain available to the composition engine
    HR(swapChain->Present(1,   // sync
                          0)); // flags
}

void BorderlessWindow::set_transparent_window(float d) {
    ::SetWindowLongPtrW(handle, GWL_STYLE, static_cast<LONG>(Style::transparent));
    ::SetWindowLongPtr(handle, GWL_EXSTYLE, static_cast<LONG>(Style::aero_borderless));

    // redraw frame
    ::SetWindowPos(handle, nullptr, 300, 100, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    ::ShowWindow(handle, SW_SHOW);
}

void BorderlessWindow::load_statics() {
    hIcon = static_cast<HICON>(LoadImage(nullptr, L"../assets/penguin.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
    if (hIcon == nullptr) {
        throw last_error("failed to load icon");
    }
}
