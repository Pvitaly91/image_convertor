#include <windows.h>
#include <shlobj.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace
{
constexpr wchar_t kWindowClassName[] = L"ImageConvertorMainWindow";
constexpr UINT WM_APP_STATUS_TEXT = WM_APP + 1;
constexpr UINT WM_APP_WORK_FINISHED = WM_APP + 2;

HWND g_hEditUrl = nullptr;
HWND g_hButtonConvert = nullptr;
HWND g_hStatus = nullptr;
bool g_isWorking = false;

void SetStatusText(const std::wstring& text)
{
    if (g_hStatus)
    {
        SetWindowTextW(g_hStatus, text.c_str());
    }
}

void PostStatusText(HWND hwnd, const std::wstring& text)
{
    auto* payload = new std::wstring(text);
    PostMessageW(hwnd, WM_APP_STATUS_TEXT, 0, reinterpret_cast<LPARAM>(payload));
}

std::wstring GetPicturesFolder()
{
    PWSTR path = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, KF_FLAG_DEFAULT, nullptr, &path)) && path)
    {
        result.assign(path);
        CoTaskMemFree(path);
    }
    return result;
}

bool IsValidUrl(const std::wstring& url)
{
    if (url.size() < 8)
    {
        return false;
    }

    const std::wstring prefixHttp = L"http://";
    const std::wstring prefixHttps = L"https://";

    if (_wcsnicmp(url.c_str(), prefixHttp.c_str(), prefixHttp.size()) == 0)
    {
        return true;
    }
    if (_wcsnicmp(url.c_str(), prefixHttps.c_str(), prefixHttps.size()) == 0)
    {
        return true;
    }
    return false;
}

void RunWorker(HWND hwnd, std::wstring url)
{
    auto status = [&](const std::wstring& message) { PostStatusText(hwnd, message); };

    status(L"Downloading...");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    status(L"Decoding...");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    status(L"Saving...");
    auto pictures = GetPicturesFolder();
    if (pictures.empty())
    {
        pictures = L".";
    }

    std::filesystem::path outputPath = std::filesystem::path(pictures) / L"test.jpg";
    std::ofstream out(outputPath, std::ios::binary);
    if (out)
    {
        static const unsigned char dummy[] = {0xFF, 0xD8, 0xFF, 0xD9};
        out.write(reinterpret_cast<const char*>(dummy), sizeof(dummy));
    }
    out.close();

    status(L"Done. Saved to:\r\n" + outputPath.wstring());
    PostMessageW(hwnd, WM_APP_WORK_FINISHED, 0, 0);
}

void StartWorker(HWND hwnd, const std::wstring& url)
{
    if (g_isWorking)
    {
        return;
    }
    g_isWorking = true;
    EnableWindow(g_hButtonConvert, FALSE);

    std::thread worker([hwnd, url]() { RunWorker(hwnd, url); });
    worker.detach();
}

void LayoutControls(HWND hwnd, int width, int height)
{
    constexpr int margin = 12;
    constexpr int controlHeight = 24;
    constexpr int buttonWidth = 120;

    int x = margin;
    int y = margin;
    int editWidth = width - (3 * margin) - buttonWidth;

    MoveWindow(g_hEditUrl, x, y, editWidth, controlHeight, TRUE);
    MoveWindow(g_hButtonConvert, x + editWidth + margin, y, buttonWidth, controlHeight, TRUE);

    y += controlHeight + margin;
    int statusHeight = height - y - margin;
    MoveWindow(g_hStatus, x, y, width - 2 * margin, statusHeight, TRUE);
}

void CreateControls(HWND hwnd)
{
    HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    g_hEditUrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                 WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(1), GetModuleHandleW(nullptr), nullptr);
    SendMessageW(g_hEditUrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    g_hButtonConvert = CreateWindowExW(0, L"BUTTON", L"Convert",
                                       WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                       0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(2), GetModuleHandleW(nullptr), nullptr);
    SendMessageW(g_hButtonConvert, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    g_hStatus = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
                                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(3), GetModuleHandleW(nullptr), nullptr);
    SendMessageW(g_hStatus, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateControls(hwnd);
        break;
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        LayoutControls(hwnd, width, height);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 2 && HIWORD(wParam) == BN_CLICKED)
        {
            wchar_t buffer[2048] = {};
            GetWindowTextW(g_hEditUrl, buffer, static_cast<int>(std::size(buffer)));
            std::wstring url(buffer);
            if (!IsValidUrl(url))
            {
                SetStatusText(L"Please enter a valid http/https URL.");
                return 0;
            }
            SetStatusText(L"Starting conversion...");
            StartWorker(hwnd, url);
            return 0;
        }
        break;
    case WM_APP_STATUS_TEXT:
    {
        std::wstring* text = reinterpret_cast<std::wstring*>(lParam);
        if (text)
        {
            SetStatusText(*text);
            delete text;
        }
        break;
    }
    case WM_APP_WORK_FINISHED:
        g_isWorking = false;
        EnableWindow(g_hButtonConvert, TRUE);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return 0;
}
} // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wcex))
    {
        return 0;
    }

    HWND hwnd = CreateWindowExW(0, kWindowClassName, L"Image Convertor (stub)",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 360,
                                nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
