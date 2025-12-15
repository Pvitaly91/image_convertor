// ImageConvertor - Windows GUI Application
// Converts AVIF images from URL to JPEG format

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#include <commctrl.h>

#include <string>
#include <thread>
#include <atomic>
#include <filesystem>

// Control IDs
constexpr int ID_EDIT_URL = 101;
constexpr int ID_BUTTON_CONVERT = 102;
constexpr int ID_EDIT_STATUS = 103;

// Custom window messages for thread communication
constexpr UINT WM_UPDATE_STATUS = WM_APP + 1;
constexpr UINT WM_CONVERSION_DONE = WM_APP + 2;
constexpr UINT WM_CONVERSION_ERROR = WM_APP + 3;

// Global handles
HWND g_hEditUrl = nullptr;
HWND g_hButtonConvert = nullptr;
HWND g_hEditStatus = nullptr;
HWND g_hMainWnd = nullptr;

// Worker thread control
std::atomic<bool> g_bWorking{ false };

// Get Pictures folder path
std::wstring GetPicturesFolder()
{
    PWSTR pszPath = nullptr;
    std::wstring result;
    
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pszPath);
    if (SUCCEEDED(hr) && pszPath)
    {
        result = pszPath;
        CoTaskMemFree(pszPath);
    }
    else
    {
        // Fallback to current directory
        wchar_t buffer[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, buffer);
        result = buffer;
    }
    
    return result;
}

// Validate URL (simple check for http:// or https://)
bool ValidateUrl(const std::wstring& url)
{
    if (url.empty())
        return false;
    
    std::wstring lowerUrl = url;
    for (auto& ch : lowerUrl)
        ch = towlower(ch);
    
    return (lowerUrl.find(L"http://") == 0 || lowerUrl.find(L"https://") == 0);
}

// Update status text (thread-safe)
void PostStatus(const std::wstring& status)
{
    // Allocate a copy of the string for the message
    wchar_t* pStr = new wchar_t[status.length() + 1];
    wcscpy_s(pStr, status.length() + 1, status.c_str());
    PostMessage(g_hMainWnd, WM_UPDATE_STATUS, 0, reinterpret_cast<LPARAM>(pStr));
}

// Worker thread function (conversion pipeline stub)
void WorkerThreadProc(std::wstring url)
{
    // Step 1: Downloading
    PostStatus(L"Downloading...");
    Sleep(500); // Simulate download time
    
    // TODO: Implement actual WinHTTP download here
    // - Use WinHttpOpen, WinHttpConnect, WinHttpOpenRequest, WinHttpSendRequest
    // - Download the AVIF file to memory buffer
    
    // Step 2: Decoding
    PostStatus(L"Decoding...");
    Sleep(500); // Simulate decode time
    
    // TODO: Implement libavif decoding here
    // - Use avifDecoder* functions to decode AVIF data
    // - Convert to RGB pixel data
    
    // Step 3: Saving
    PostStatus(L"Saving...");
    Sleep(500); // Simulate save time
    
    // Get output path
    std::wstring picturesPath = GetPicturesFolder();
    std::filesystem::path outputPath = std::filesystem::path(picturesPath) / L"converted_image.jpg";
    
    // TODO: Implement libjpeg-turbo encoding here
    // - Use tjCompress2 to encode RGB data to JPEG
    // - Write to file
    
    // For now, create a dummy file to verify write access
    HANDLE hFile = CreateFileW(
        outputPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // Write dummy JPEG header (minimal valid JPEG)
        // FFD8 FFE0 - JPEG SOI and APP0 marker
        const unsigned char dummyJpeg[] = {
            0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
            0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43,
            0x00, 0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09,
            0x09, 0x08, 0x0A, 0x0C, 0x14, 0xFF, 0xD9
        };
        DWORD bytesWritten = 0;
        WriteFile(hFile, dummyJpeg, sizeof(dummyJpeg), &bytesWritten, nullptr);
        CloseHandle(hFile);
        
        // Step 4: Done
        std::wstring doneMsg = L"Done!\r\n\r\nOutput file:\r\n" + outputPath.wstring();
        PostStatus(doneMsg);
        PostMessage(g_hMainWnd, WM_CONVERSION_DONE, 0, 0);
    }
    else
    {
        DWORD error = GetLastError();
        std::wstring errorMsg = L"Error: Failed to create output file.\r\nError code: " + std::to_wstring(error);
        PostStatus(errorMsg);
        PostMessage(g_hMainWnd, WM_CONVERSION_ERROR, 0, 0);
    }
    
    g_bWorking = false;
}

// Start conversion
void StartConversion()
{
    if (g_bWorking)
    {
        MessageBoxW(g_hMainWnd, L"Conversion already in progress!", L"Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Get URL from edit control
    int len = GetWindowTextLengthW(g_hEditUrl);
    if (len == 0)
    {
        MessageBoxW(g_hMainWnd, L"Please enter a URL.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    std::wstring url(len + 1, L'\0');
    GetWindowTextW(g_hEditUrl, &url[0], len + 1);
    url.resize(len);
    
    // Validate URL
    if (!ValidateUrl(url))
    {
        MessageBoxW(g_hMainWnd, L"Invalid URL. Please enter a valid http:// or https:// URL.", 
                    L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Clear status and disable button
    SetWindowTextW(g_hEditStatus, L"Starting conversion...");
    EnableWindow(g_hButtonConvert, FALSE);
    
    // Start worker thread
    g_bWorking = true;
    std::thread worker(WorkerThreadProc, url);
    worker.detach();
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Create URL label
        CreateWindowW(
            L"STATIC", L"Image URL:",
            WS_CHILD | WS_VISIBLE,
            10, 15, 80, 20,
            hWnd, nullptr, nullptr, nullptr
        );
        
        // Create URL edit control
        g_hEditUrl = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT", L"https://example.com/image.avif",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            100, 10, 370, 25,
            hWnd, reinterpret_cast<HMENU>(ID_EDIT_URL), nullptr, nullptr
        );
        
        // Create Convert button
        g_hButtonConvert = CreateWindowW(
            L"BUTTON", L"Convert",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            480, 10, 100, 25,
            hWnd, reinterpret_cast<HMENU>(ID_BUTTON_CONVERT), nullptr, nullptr
        );
        
        // Create Status label
        CreateWindowW(
            L"STATIC", L"Status:",
            WS_CHILD | WS_VISIBLE,
            10, 50, 60, 20,
            hWnd, nullptr, nullptr, nullptr
        );
        
        // Create Status edit control (multi-line, read-only)
        g_hEditStatus = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT", L"Ready. Enter a URL and click Convert.",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 75, 570, 180,
            hWnd, reinterpret_cast<HMENU>(ID_EDIT_STATUS), nullptr, nullptr
        );
        
        // Set default font for all controls
        HFONT hFont = CreateFontW(
            -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
        
        EnumChildWindows(hWnd, [](HWND hChild, LPARAM lParam) -> BOOL {
            SendMessage(hChild, WM_SETFONT, lParam, TRUE);
            return TRUE;
        }, reinterpret_cast<LPARAM>(hFont));
        
        return 0;
    }
    
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_BUTTON_CONVERT:
            StartConversion();
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
    
    case WM_UPDATE_STATUS:
    {
        // Update status text from worker thread
        wchar_t* pStr = reinterpret_cast<wchar_t*>(lParam);
        if (pStr)
        {
            SetWindowTextW(g_hEditStatus, pStr);
            delete[] pStr;
        }
        return 0;
    }
    
    case WM_CONVERSION_DONE:
    case WM_CONVERSION_ERROR:
    {
        // Re-enable the button
        EnableWindow(g_hButtonConvert, TRUE);
        return 0;
    }
    
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        
    case WM_GETMINMAXINFO:
    {
        // Set minimum window size
        MINMAXINFO* pMMI = reinterpret_cast<MINMAXINFO*>(lParam);
        pMMI->ptMinTrackSize.x = 620;
        pMMI->ptMinTrackSize.y = 320;
        return 0;
    }
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Application entry point
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    // Initialize COM (required for SHGetKnownFolderPath)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Register window class
    const wchar_t CLASS_NAME[] = L"ImageConvertorWindowClass";
    
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wcex))
    {
        MessageBoxW(nullptr, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
    
    // Create main window
    g_hMainWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Image Convertor - AVIF to JPEG",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 320,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (!g_hMainWnd)
    {
        MessageBoxW(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }
    
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        // Handle Tab key navigation
        if (!IsDialogMessage(g_hMainWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
