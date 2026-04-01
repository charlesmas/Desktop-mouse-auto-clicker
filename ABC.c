#include <windows.h>
#include <shellapi.h>

#define TIMER_ID 100
#define WM_TRAYICON (WM_USER + 1)

BOOL running = FALSE;
HWND hWndMain;
HWND hStatus;
NOTIFYICONDATAW nid = {0}; // 使用 Unicode 版本

// 模拟左右键点击
void ClickMouse() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    // 左键点击
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(10);

    // 右键点击
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

// 托盘菜单
void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    HMENU hMenu = CreatePopupMenu();
    GetCursorPos(&pt);

    InsertMenuW(hMenu, -1, MF_BYPOSITION, 1, running ? L"停止" : L"开始");
    InsertMenuW(hMenu, -1, MF_BYPOSITION, 2, L"退出");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            hStatus = CreateWindowW(L"STATIC", L"状态：已停止",
                                    WS_VISIBLE | WS_CHILD,
                                    20, 20, 200, 25,
                                    hwnd, NULL, NULL, NULL);
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID && running) {
                ClickMouse();
            }
            break;

        case WM_HOTKEY:
            if (wParam == 1) { // F8 热键
                if (!running) {
                    SetTimer(hwnd, TIMER_ID, 200, NULL);
                    running = TRUE;
                    SetWindowTextW(hStatus, L"状态：运行中");
                } else {
                    KillTimer(hwnd, TIMER_ID);
                    running = FALSE;
                    SetWindowTextW(hStatus, L"状态：已停止");
                }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1:
                    if (!running) {
                        SetTimer(hwnd, TIMER_ID, 200, NULL);
                        running = TRUE;
                        SetWindowTextW(hStatus, L"状态：运行中");
                    } else {
                        KillTimer(hwnd, TIMER_ID);
                        running = FALSE;
                        SetWindowTextW(hStatus, L"状态：已停止");
                    }
                    break;
                case 2:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                if (IsWindowVisible(hwnd)) {
                    ShowWindow(hwnd, SW_HIDE);
                } else {
                    ShowWindow(hwnd, SW_SHOW);
                }
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd); // 右键退出真正销毁窗口
            break;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID);
            Shell_NotifyIconW(NIM_DELETE, &nid);
            UnregisterHotKey(hwnd, 1);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 入口 main
int main() {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ClickerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"注册窗口类失败！", L"错误", MB_ICONERROR);
        return 0;
    }

    hWndMain = CreateWindowW(L"ClickerClass",
                             L"后台鼠标连点器",
                             WS_OVERLAPPEDWINDOW & ~WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             300, 150,
                             NULL, NULL, wc.hInstance, NULL);

    if (!hWndMain) {
        MessageBoxW(NULL, L"创建窗口失败！", L"错误", MB_ICONERROR);
        return 0;
    }

    // 注册全局热键 F8
    if (!RegisterHotKey(hWndMain, 1, 0, VK_F8)) {
        MessageBoxW(NULL, L"鼠标连点器已启动。", L"提示窗口", MB_ICONERROR);
        return 0;
    }

    // 托盘图标
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWndMain;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy(nid.szTip, L"鼠标连点器"); // Unicode 字符串
    Shell_NotifyIconW(NIM_ADD, &nid);

    ShowWindow(hWndMain, SW_HIDE); // 程序启动时隐藏窗口

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
