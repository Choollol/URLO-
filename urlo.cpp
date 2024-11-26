#include <windows.h>
#include <winuser.h>

#include <filesystem>
#include <fstream>
#include <vector>

#ifndef NDEBUG
#include <iostream>
#endif

// Define class name as a const wchar_t*
#define CLASS_NAME L"URLO"
// Maximum number of buttons/files allowed
#define MAX_BUTTON_COUNT 9
// Button dimensions
#define BUTTON_WIDTH 160
#define BUTTON_HEIGHT 30
#define BUTTON_PADDING 10

// Declare functions
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void getFileNames(std::vector<std::wstring> &vec);
void openURLs(std::wstring fileName, bool doQuit);
void errorAlert(const wchar_t *message);
void errorAlert(const std::wstring &message);

// Type is char because the maximum button count will never be above 255
unsigned char buttonCount;
// Collection of handles to the button windows
std::vector<HWND> hWndButtons;
// Handle to parent window
HWND hWndParent;
// Collection of the names of .txt files in the same directory as the .exe
std::vector<std::wstring> fileNames;

// Windows entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    // Store names of .txt files in the same directory
    getFileNames(fileNames);

    // Set properties of the parent window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    // Register the parent window class
    if (!RegisterClassW(&wc)) {
        errorAlert(L"Could not register class");
        return 1;
    }

    // Set handle to the parent window
    hWndParent = CreateWindowExW(
        0, CLASS_NAME, CLASS_NAME, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, BUTTON_WIDTH * 2, (BUTTON_HEIGHT + BUTTON_PADDING) * buttonCount + BUTTON_PADDING * 4,
        nullptr, nullptr, hInstance, nullptr);
    // Abort program if handle failed to be set
    if (!hWndParent) {
        errorAlert(L"Could not set handle to parent window failed");
        return 1;
    }

    // Center window
    RECT windowRect;
    if (!GetWindowRect(hWndParent, &windowRect)) {
        errorAlert(L"Could not get window rect of parent window");
        return 1;
    }
    WINBOOL successStatus = SetWindowPos(hWndParent, nullptr, (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2,
                                         (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    if (!successStatus) {
        errorAlert(L"Could not set parent window position");
        return 1;
    }

    // Show parent window
    ShowWindow(hWndParent, nCmdShow);

    // Get dimensions of the client area
    RECT clientRect;
    if (!GetClientRect(hWndParent, &clientRect)) {
        errorAlert(L"Could not get client rect of parent window");
        return 1;
    }

    // Calculate button positions
    int buttonX = (clientRect.right - BUTTON_WIDTH) / 2;
    int topButtonY = clientRect.top + BUTTON_PADDING;

    // Add buttons to collection and show each button window
    for (size_t i = 0; i < buttonCount; ++i) {
        HWND hWndButton = CreateWindowExW(
            0, L"BUTTON", fileNames[i].c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            buttonX, topButtonY + (BUTTON_HEIGHT + BUTTON_PADDING) * i, BUTTON_WIDTH, BUTTON_HEIGHT,
            hWndParent, nullptr, hInstance, nullptr);

        // Check if creating the button window failed
        if (hWndButton == nullptr) {
            errorAlert(L"Could not create button window " + (i + '0'));
            return 1;
        }

        hWndButtons.push_back(hWndButton);

        ShowWindow(hWndButton, nCmdShow);
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

// Define window procedure function
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Handle message, if it should be
    switch (uMsg) {
        // Close parent window
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        // Paint the window
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // Fill background with color based on system's pallette
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hWnd, &ps);
            return 0;
        }
        // A button has been clicked
        case WM_COMMAND:
            // Find which button was clicked
            for (size_t i = 0; i < buttonCount; ++i) {
                if (reinterpret_cast<HWND>(lParam) == hWndButtons[i]) {
                    // Open the URLs stored in the file name associated with the clicked button
                    openURLs(fileNames[i], true);
                    return 0;
                }
            }
    }
    // Pass unhandled messages to the default window procedure function
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Get all the file names in the same directory as the .exe
void getFileNames(std::vector<std::wstring> &fileNames) {
    // Reset button count
    buttonCount = 0;
    // Iterate over each file in the same directory as the .exe
    for (const std::filesystem::directory_entry &dirEntry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        // Check that the file being iterated over is a .txt file
        if (dirEntry.path().extension() == L".txt") {
            // If the maximum number of files have been read, return early
            if (buttonCount >= MAX_BUTTON_COUNT) {
                errorAlert(L"Only " + std::to_wstring(MAX_BUTTON_COUNT) + L" files will be shown");
                return;
            }

            fileNames.push_back(dirEntry.path().filename().replace_extension(""));
            // Count the number of .txt files
            ++buttonCount;
        }
    }
}

// Open the urls stored in the .txt file with the specified file name
void openURLs(std::wstring fileName, bool doQuit) {
    // Open the .txt file with the given name
    std::wifstream file((fileName + L".txt").c_str());

    unsigned lineCount = 0;
    std::wstring line;
    // Read each line in the opened file
    while (std::getline(file, line)) {
        // Increment line count
        ++lineCount;
        // Try to open the URL
        HINSTANCE successStatus = ShellExecuteW(nullptr, nullptr, line.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        // Check if the URL was opened
        if (reinterpret_cast<INT_PTR>(successStatus) <= 32) {
            errorAlert(L"Could not open URL:\n" + line + L"\nin " + fileName + L" at line " + std::to_wstring(lineCount));
        }
    }
    // Close the file, though slightly unnecessary since file will be closed when it falls out of scope anyway
    file.close();

    // Check if the file was empty
    if (lineCount == 0) {
        errorAlert(fileName + L" is empty");
    }

    // Check if the program should quit after opening the URLs
    if (doQuit) {
        PostQuitMessage(0);
    }
}

// Shows a popup window with an error message
void errorAlert(const wchar_t *message) {
    // Show error window
    MessageBoxW(hWndParent, message, L"Error", MB_OK);
}

// Overloaded errorAlert for wstring parameters
void errorAlert(const std::wstring &message) {
    // Show error window
    MessageBoxW(hWndParent, message.c_str(), L"Error", MB_OK);
}