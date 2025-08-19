#define NO_GODOT
#define FAC_WINGUI

#define POLL_WINDOWS_IN_SFTP_THREAD

#include "cli_parser.h"

#include <windows.h>
#include <uxtheme.h>
#include "default_access.h"

#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <tlhelp32.h>
#include <tchar.h>
#include <psapi.h>


#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")


//I made this part c++17, it was easier to get going right away
AccessCoordinator gCoordinator;
bool gRunning = true;
bool gOnlyCommandLine = false;
#if defined(FAC_SIMPLE_GUI)
bool gSimpleGui = true;
#else
bool gSimpleGui = false;
#endif

#undef log_info
#undef log_warning
#undef log_error
#define log_info( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_warning( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_error( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL), __VA_ARGS__)


namespace fs = std::filesystem;

DWORD GetParentProcessId(DWORD pid) {
    DWORD ppid = (DWORD)-1;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry = { 0 };
        entry.dwSize = sizeof(entry);
        if (Process32First(snapshot, &entry)) {
            do {
                if (entry.th32ProcessID == pid) {
                    ppid = entry.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return ppid;
}

String GetProcessName(DWORD pid) {
    String name = "<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        char buffer[MAX_PATH];
        if (GetModuleBaseName(hProcess, NULL, buffer, MAX_PATH)) {
            name = buffer;
        }
        CloseHandle(hProcess);
    }
    return name;
}

bool is_launched_from_cmd()
{
  DWORD myPid = GetCurrentProcessId();
  DWORD parentPid = GetParentProcessId(myPid);
  String parentName = GetProcessName(parentPid);
  return parentName == "cmd.exe";
}

bool ensure_path_exists(fs::path const& path)
{
  if (!fs::exists(path))
  {
    std::error_code ec;
    if (fs::create_directories(path, ec))
    {
      std::cout << "Created directory: " << path << '\n';
    }
    else
    {
      std::cerr << "Failed to create directory: " << ec.message() << '\n';
      return false;
    }
  }

  return true;
}


bool save_config()
{
  char const* appData = std::getenv("APPDATA");
  if (!appData)
  {
    std::cerr << "APPDATA environment variable not found.\n";
    appData = "~/.local/share";
  }

  if (!gCoordinator.is_current_base_config_valid())
  {
    std::cout << "- Error: current config is not valid, not writing to access.config.";
    return false;
  }

  fs::path targetPath = fs::path(appData) / "Godot" / "app_userdata" / "file_access_coordinator";
  fs::path filePath = targetPath / "access.config";

  ensure_path_exists(targetPath);

  std::ofstream configFileWriteStream(filePath);
  if (configFileWriteStream)
  {
    configFileWriteStream << "filepath=" << gCoordinator.mFullLocalPath.str << '\n';
    configFileWriteStream << "user=" << gCoordinator.mUser.str << '\n';
    configFileWriteStream << "sshHostname=" << gCoordinator.mSshHostname.str << '\n';
    configFileWriteStream << "sshUsername=" << gCoordinator.mSshUsername.str << '\n';
    configFileWriteStream << "sshPassword=" << gCoordinator.mSshPassword.str << '\n';
    configFileWriteStream << "remoteBaseDir=" << gCoordinator.mRemoteBaseDir.str << '\n';
    return true;
  }

  return false;
}


bool load_config(std::string& filepath, std::string& user, std::string& sshHostname, std::string& sshUsername, std::string& sshPassword, std::string& remoteBaseDir)
{
  char const* appData = std::getenv("APPDATA");
  if (!appData)
  {
    std::cerr << "APPDATA environment variable not found.\n";
    appData = "~/.local/share";
  }

  fs::path targetPath = fs::path(appData) / "Godot" / "app_userdata" / "file_access_coordinator";
  fs::path filePath = targetPath / "access.config";

  ensure_path_exists(targetPath);

  if (fs::exists(filePath))
  {
    std::ifstream configFile(filePath);
    if (!configFile)
    {
      std::cerr << "Failed to open access.config for writing.\n";
      return false;
    }

    std::string line;
    while(std::getline(configFile, line))
    {
      if (string_begins_with(line.c_str(), "filepath="))
      {
        filepath = line.substr(9);
      }
      else if (string_begins_with(line.c_str(), "user="))
      {
        user = line.substr(5);
      }
      else if (string_begins_with(line.c_str(), "sshHostname="))
      {
        sshHostname = line.substr(12);
      }
      else if (string_begins_with(line.c_str(), "sshUsername="))
      {
        sshUsername = line.substr(12);
      }
      else if (string_begins_with(line.c_str(), "sshPassword="))
      {
        sshPassword = line.substr(12);
      }
      else if (string_begins_with(line.c_str(), "remoteBaseDir="))
      {
        remoteBaseDir = line.substr(14);
      }
    }

    return true;
  }

  return false;
}

bool open_file_dialog(HWND hwndOwner, char* buffer, s64 bufferSize) {
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = (DWORD)bufferSize;
    ofn.lpstrFilter = "Database Files\0*.sdb\0Text Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Select a File";

    return GetOpenFileName(&ofn);
}

HWND hButtonUpload;
HWND hButtonDownload;
HWND hButtonReserve;
HWND hButtonRelease;
HWND hButtonForceRelease;
HWND hButtonSaveConfig;
HWND hButtonOpenFile;

HWND hMainWindow;
HWND hUserTextbox;
HWND hFileTextbox;
HWND hOutputTextbox;

HWND hReserveStateLabel;

enum class HwndId : int
{
    BUTTON_MIN = 0,
    Button_Upload,
    Button_Download,
    Button_Reserve,
    Button_Release,
    Button_ForceRelease,
    Button_SaveConfig,
    Button_OpenFile,
    BUTTON_MAX,
    TEXT_MIN,
    Text_Output,
    TEXT_MAX,
    LABEL_MIN,
    Label_ReserveState,
    LABEL_MAX,
};

char const* get_button_text(int buttonId)
{
    switch(buttonId)
    {
        case HwndId::Button_Upload: return "Upload";
        case HwndId::Button_Download: return "Download";
        case HwndId::Button_Reserve: return "Reserve";
        case HwndId::Button_Release: return "Release";
        case HwndId::Button_ForceRelease: return "Force Release";
        case HwndId::Button_SaveConfig: return "Save Config";
    }
    return "";
}

#define UI_PADDING 2

#if defined(FAC_SIMPLE_GUI)
#define COLUMN_0_WIDTH 250
#define ROW_0_HEIGHT 30
#else
#define COLUMN_0_WIDTH 100
#define ROW_0_HEIGHT 20
#endif
#define COLUMN_1_WIDTH 40
#define COLUMN_2_WIDTH 420
#define COLUMN_3_WIDTH 100

#define ROW_1_HEIGHT 20
#define ROW_2_HEIGHT 500
#define ROW_3_HEIGHT 20
#define ROW_4_HEIGHT 0
#define ROW_5_HEIGHT 0

#define COLUMN_0_OFFSET UI_PADDING
#define COLUMN_1_OFFSET COLUMN_0_WIDTH + COLUMN_0_OFFSET + UI_PADDING
#define COLUMN_2_OFFSET COLUMN_1_WIDTH + COLUMN_1_OFFSET + UI_PADDING
#define COLUMN_3_OFFSET COLUMN_2_WIDTH + COLUMN_2_OFFSET + UI_PADDING
#define COLUMN_4_OFFSET COLUMN_3_WIDTH + COLUMN_3_OFFSET + UI_PADDING

#define ROW_0_OFFSET UI_PADDING
#define ROW_1_OFFSET ROW_0_HEIGHT + ROW_0_OFFSET + UI_PADDING
#define ROW_2_OFFSET ROW_1_HEIGHT + ROW_1_OFFSET + UI_PADDING
#define ROW_3_OFFSET ROW_2_HEIGHT + ROW_2_OFFSET + UI_PADDING
#define ROW_4_OFFSET ROW_3_HEIGHT + ROW_3_OFFSET + UI_PADDING
#define ROW_5_OFFSET ROW_4_HEIGHT + ROW_4_OFFSET + UI_PADDING
#define ROW_6_OFFSET ROW_5_HEIGHT + ROW_5_OFFSET + UI_PADDING

#if defined(FAC_SIMPLE_GUI)
#define WINDOW_WIDTH  COLUMN_0_WIDTH * 2 + UI_PADDING
#define WINDOW_HEIGHT ROW_1_OFFSET
#else
#define WINDOW_WIDTH  COLUMN_4_OFFSET
#define WINDOW_HEIGHT ROW_3_OFFSET
#endif

#define BUTTON_STYLE BS_PUSHBUTTON
// #define BUTTON_STYLE BS_OWNERDRAW

HBRUSH hTextBoxBrush;
HBRUSH hButtonBrush;
HBRUSH hWindowBrush;
COLORREF textBkColor = RGB(30,30,30);
COLORREF textForegroundColor = RGB(200,200,200);
COLORREF buttonBkColor = RGB(75, 75, 75);
COLORREF buttonForegroundColor = RGB(255, 255, 255);
COLORREF windowBkColor = RGB(50, 50, 50);
HFONT hOutputFont;

ReserveState reserveState = ReserveState::UNKNOWN;
//TODO
bool reservedFileExistsLocally = true;
bool reservedFileSizeDifferent = false;
String reservedFileRemoteOwner;

LRESULT CALLBACK window_proc_essentials(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_DESTROY:
        DeleteObject(hTextBoxBrush);
        DeleteObject(hWindowBrush);
        DeleteObject(hButtonBrush);
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      SetTextColor(hdc, textForegroundColor);
      SetBkColor(hdc, windowBkColor);
      if (!gSimpleGui)
      {

        char const* lblUser = "User: ";
        char const* lblFile = "File: ";
        RECT rectRow0 = { COLUMN_1_OFFSET, ROW_0_OFFSET, COLUMN_1_OFFSET + COLUMN_1_WIDTH, ROW_0_OFFSET + ROW_0_HEIGHT };
        RECT rectRow1 = { COLUMN_1_OFFSET, ROW_1_OFFSET, COLUMN_1_OFFSET + COLUMN_1_WIDTH, ROW_1_OFFSET + ROW_1_HEIGHT };

        DrawText(hdc, lblUser, -1, &rectRow0, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
        DrawText(hdc, lblFile, -1, &rectRow1, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

      }
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID > (int)HwndId::BUTTON_MIN && dis->CtlID < (int)HwndId::BUTTON_MAX) {
            // Fill background
            FillRect(dis->hDC, &dis->rcItem, hButtonBrush);
            // Draw button text
            SetTextColor(dis->hDC, buttonForegroundColor);
            SetBkColor(dis->hDC, buttonBkColor);
            //SetBkMode(dis->hDC, TRANSPARENT);
            DrawText(dis->hDC, get_button_text(dis->CtlID), -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, textForegroundColor);
        SetBkColor(hdc, textBkColor);
        if((HWND)lParam == hButtonUpload
        || (HWND)lParam == hButtonDownload
        || (HWND)lParam == hButtonReserve
        || (HWND)lParam == hButtonRelease
        || (HWND)lParam == hButtonForceRelease
        || (HWND)lParam == hButtonSaveConfig)
        {
            SetTextColor(hdc, buttonForegroundColor);
            SetBkColor(hdc, buttonBkColor);
            return (INT_PTR)hButtonBrush;
        }
        return (INT_PTR)hTextBoxBrush;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void create_gui(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  hTextBoxBrush = CreateSolidBrush(textBkColor);
  hButtonBrush = CreateSolidBrush(buttonBkColor);
  if (gSimpleGui)
  {
    hButtonUpload = CreateWindow(
      "BUTTON", "Bearbeiten Starten",
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
      COLUMN_0_OFFSET, ROW_0_OFFSET, COLUMN_0_WIDTH, ROW_0_HEIGHT,
      hwnd, (HMENU)HwndId::Button_Download, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
    hButtonDownload = CreateWindow(
        "BUTTON", "Bearbeiten Beenden",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_1_OFFSET, ROW_0_OFFSET, COLUMN_0_WIDTH, ROW_0_HEIGHT,
        hwnd, (HMENU)HwndId::Button_Upload, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
    hReserveStateLabel = CreateWindow(
      "STATIC",
      "",
      WS_VISIBLE | WS_CHILD,
      COLUMN_0_OFFSET + UI_PADDING, ROW_1_OFFSET, COLUMN_1_OFFSET + COLUMN_0_WIDTH, ROW_1_HEIGHT,
      hwnd,
      (HMENU)HwndId::Label_ReserveState,
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
  }
  else
  {
    SetWindowTheme(hButtonUpload, L"", L"");
    SetWindowTheme(hButtonDownload, L"", L"");
    SetWindowTheme(hButtonReserve, L"", L"");
    SetWindowTheme(hButtonRelease, L"", L"");
    SetWindowTheme(hButtonForceRelease, L"", L"");
    SetWindowTheme(hButtonSaveConfig, L"", L"");
    SetWindowTheme(hButtonOpenFile, L"", L"");

    // BUTTONS
    hButtonUpload = CreateWindow(
        "BUTTON", "Upload",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_0_OFFSET, ROW_0_OFFSET, COLUMN_0_WIDTH, ROW_0_HEIGHT,
        hwnd, (HMENU)HwndId::Button_Upload, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hButtonDownload = CreateWindow(
        "BUTTON", "Download",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_0_OFFSET, ROW_1_OFFSET, COLUMN_0_WIDTH, ROW_1_HEIGHT,
        hwnd, (HMENU)HwndId::Button_Download, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hButtonReserve = CreateWindow(
        "BUTTON", "Reserve",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_3_OFFSET - 2 * COLUMN_1_OFFSET - 4 * UI_PADDING, ROW_3_OFFSET, COLUMN_0_WIDTH, ROW_3_HEIGHT,
        hwnd, (HMENU)HwndId::Button_Reserve, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hButtonRelease = CreateWindow(
        "BUTTON", "Release",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_3_OFFSET - COLUMN_1_OFFSET - 3 * UI_PADDING, ROW_3_OFFSET, COLUMN_0_WIDTH, ROW_3_HEIGHT,
        hwnd, (HMENU)HwndId::Button_Release, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hButtonForceRelease = CreateWindow(
        "BUTTON", "Force Release",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_3_OFFSET, ROW_3_OFFSET, COLUMN_0_WIDTH, ROW_3_HEIGHT,
        hwnd, (HMENU)HwndId::Button_ForceRelease, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    // hButtonSaveConfig = CreateWindow(
    //     "BUTTON", "Save Config",
    //     WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
    //     COLUMN_0_OFFSET, ROW_5_OFFSET, COLUMN_0_WIDTH, ROW_5_HEIGHT,
    //     hwnd, (HMENU)HwndId::Button_SaveConfig, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    // );
    hButtonOpenFile = CreateWindow(
        "BUTTON", "Browse Files",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BUTTON_STYLE,
        COLUMN_3_OFFSET, ROW_1_OFFSET, COLUMN_3_WIDTH, ROW_1_HEIGHT,
        hwnd, (HMENU)HwndId::Button_OpenFile, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );

    //TEXT FIELDS
    hUserTextbox = CreateWindowEx(
        0, "EDIT", gCoordinator.mUser.get_data(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        COLUMN_2_OFFSET, ROW_0_OFFSET, COLUMN_2_WIDTH, ROW_0_HEIGHT,  // x, y, width, height
        hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hFileTextbox = CreateWindowEx(
        0, "EDIT", gCoordinator.mFullLocalPath.get_data(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        COLUMN_2_OFFSET, ROW_1_OFFSET, COLUMN_2_WIDTH, ROW_1_HEIGHT,  // x, y, width, height
        hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
    );
    hOutputTextbox = CreateWindowEx(
        0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
        COLUMN_0_OFFSET, ROW_2_OFFSET, WINDOW_WIDTH, ROW_2_HEIGHT,
        hwnd, (HMENU)HwndId::Text_Output, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

    //FONT
    hOutputFont = CreateFont(
        -MulDiv(10, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE, "Consolas");
    SendMessage(hOutputTextbox, WM_SETFONT, (WPARAM)hOutputFont, TRUE);

    //LABEL
    hReserveStateLabel = CreateWindow(
    "STATIC",
    "",
    WS_VISIBLE | WS_CHILD,
    COLUMN_0_OFFSET, ROW_3_OFFSET,
    COLUMN_3_OFFSET - 2 * COLUMN_1_OFFSET - 4 * UI_PADDING, ROW_3_HEIGHT,
    hwnd,
    (HMENU)HwndId::Label_ReserveState,
    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
  }
}

bool gOnlyProcEssentials = false;
LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    char buffer[BSE_STACK_BUFFER_SMALL];
    if (gOnlyProcEssentials)
    {
        return window_proc_essentials(hwnd, uMsg, wParam, lParam);
    }

    switch (uMsg) {
        case WM_CREATE:
        {
            create_gui(hwnd, wParam, lParam);
            return 0;
        }

        case WM_COMMAND:
        {
            buffer[0] = '\0';
            GetWindowText(hUserTextbox, buffer, sizeof(buffer));
            if (buffer[0]) gCoordinator.mUser = buffer;
            buffer[0] = '\0';
            GetWindowText(hFileTextbox, buffer, sizeof(buffer));
            if (buffer[0]) gCoordinator.set_filepath(String(buffer));
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case (int)HwndId::Button_Upload:
                    {
                      if (gSimpleGui)
                      {
                        if (reserveState == ReserveState::UNKNOWN || reserveState == ReserveState::RESERVED_BY_ME)
                        {
                          gCoordinator.upload();
                        }
                        else
                        {
                          buffer[0] = '\0';
                          string_format(buffer, sizeof(buffer), "Datenbank\n", gCoordinator.mFullLocalPath.get_data(), "\nist nicht in bearbeitung.");
                          int confirmOverwrite = MessageBoxA(
                          hMainWindow,
                          buffer,
                          "Datenbank nicht in bearbeitung.",
                          MB_OK | MB_ICONQUESTION | MB_TOPMOST);
                        }
                      }
                      else
                      {
                        gCoordinator.upload();
                      }
                        break;
                    }
                    case (int)HwndId::Button_Download:
                    {
                      if (gSimpleGui && reserveState == ReserveState::RESERVED_BY_ME)
                      {
                        int confirmOverwrite = MessageBoxW(
                          hMainWindow,
                          L"Datenbank ist zurzeit schon in bearbeitung.\nLokale Version ersetzen?",
                          L"Datenbank sicher ersetzen?",
                          MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
                        if (confirmOverwrite == IDYES)
                        {
                          gCoordinator.download();
                        }
                      }
                      else
                      {
                        gCoordinator.download();
                      }

                      break;
                    }
                    case (int)HwndId::Button_Reserve:
                    {
                        gCoordinator.reserve();
                        break;
                    }
                    case (int)HwndId::Button_Release:
                    {
                        gCoordinator.release(false);
                        break;
                    }
                    case (int)HwndId::Button_ForceRelease:
                    {
                        gCoordinator.release(true);
                        break;
                    }
                    case (int)HwndId::Button_SaveConfig:
                    {
                        save_config();
                        string_format(buffer, sizeof(buffer), "Config Saved.\nSaved user: ", gCoordinator.mUser.get_data(),"\nSaved file: ", gCoordinator.mFullLocalPath.get_data() );
                        MessageBox(hwnd, buffer, "Config Saved!", MB_OK);
                        break;
                    }
                    case (int)HwndId::Button_OpenFile:
                    {
                        buffer[0] = '\n';
                        string_format(buffer, sizeof(buffer), gCoordinator.mFullLocalPath.get_data());
                        if (open_file_dialog(hMainWindow, buffer, sizeof(buffer)))
                        {
                            gCoordinator.set_filepath(buffer);
                            SetWindowText(hFileTextbox, buffer);
                            save_config();
                        }
                        break;
                    }
                    default:
                    {}
                }
            }
            return 0;
        }
    }

    return window_proc_essentials(hwnd, uMsg, wParam, lParam);
}

bool poll_win_message(bool onlyEssentials)
{
    gOnlyProcEssentials = onlyEssentials;
    MSG msg = {};
    if (GetMessage(&msg, NULL, 0, 0)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return true;
    }

    return false;
}

s32 lastNewlineIndex = 0;

void fetch_new_log_callback()
{
  if(gOnlyCommandLine)
  {
    return;
  }

  char const* log = gCoordinator.fetch_output().get_data();
  if (log && *log)
  {
    if (gSimpleGui)
    {
      bool overwriteLastLine = log[0] == '\r' && log[1] != '\n';
      if (overwriteLastLine)
      {
        SetWindowText(hReserveStateLabel, log + 1);
      }
    }
    else
    {
      int len = GetWindowTextLength(hOutputTextbox);
      int start = len;
      bool overwriteLastLine = log[0] == '\r' && log[1] != '\n';
      char const* last = string_find_last(log, '\n');
      if (last)
      {
        lastNewlineIndex = len + int(last - log);
      }

      if (overwriteLastLine)
      {
        start = lastNewlineIndex + 1;
      }

      SendMessage(hOutputTextbox, EM_SETSEL, start, len);
      SendMessage(hOutputTextbox, EM_REPLACESEL, FALSE, (LPARAM)log);
    }
  }
}

bool confirm_dialog_callback(char const* title, char const* message)
{
  if (gOnlyCommandLine)
  {
    std::string input;
    std::cout << message;
    std::cout << " (y/n): ";
    std::getline(std::cin, input);
    return input == "y" || input == "Y";
  }
  else
  {
    if (gSimpleGui)
    {
      return true;
    }
    else
    {
      int result = MessageBox(
        hMainWindow,
        message,
        title,
        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
      return result == IDYES;
    }
  }
}

int loop_wait_for_expression()
{
  while(gRunning)
  {
    std::string input;
    std::cout << "----------------------------------------------\nCommand: ";
    std::getline(std::cin, input);
    if (!evaluate_expression(input.c_str(), gCoordinator)) gRunning = false;
  }

  return 0;
}

int main_console(LPSTR commandLine)
{
  int argc;
  LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argvW != NULL)
  {
    char* argv[128];
    char buffer[BSE_STACK_BUFFER_MEDIUM];
    int bytesWritten = 0;
    for (int i = 0; i < argc; ++i)
    {
      argv[i] = buffer + bytesWritten;
      bytesWritten += WideCharToMultiByte(CP_ACP, 0, argvW[i], -1, buffer + bytesWritten, sizeof(buffer) - bytesWritten, NULL, NULL);
    }

    LocalFree(argvW);

    for (int i = 1; i < argc; ++i)
    {
      if (!evaluate_expression(argv[i]), gCoordinator) gRunning = false;
    }

    loop_wait_for_expression();
    return 0;
  }

  return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int nCmdShow)
{
  gCoordinator.mConfirmationDialogCallback = &confirm_dialog_callback;

  std::string filepath;
  std::string user;
  std::string sshHostname;
  std::string sshUsername;
  std::string sshPassword;
  std::string remoteBaseDir;
  bool needToLoadDefault = !load_config(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
  gOnlyCommandLine = is_launched_from_cmd();
#if defined(FAC_DEBUG)
  if (AllocConsole()) {
      FILE* fp;
      freopen_s(&fp, "CONOUT$", "w", stdout);
      freopen_s(&fp, "CONOUT$", "w", stderr);
      freopen_s(&fp, "CONIN$", "r", stdin);

      std::ios::sync_with_stdio(true);
  }
#endif

  if (gOnlyCommandLine)
  {
    if (AllocConsole()) {
      FILE* fp;
      freopen_s(&fp, "CONOUT$", "w", stdout);
      freopen_s(&fp, "CONOUT$", "w", stderr);
      freopen_s(&fp, "CONIN$", "r", stdin);

      std::ios::sync_with_stdio(true);
    }
  }
  else
  {
    const char CLASS_NAME[] = "File Access Coordinator";

    gCoordinator.mNewLogSignal = &fetch_new_log_callback;
    gCoordinator.mWindowMessageCallback = &poll_win_message;
    hWindowBrush = CreateSolidBrush(windowBkColor);
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hWindowBrush;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, true, WS_EX_OVERLAPPEDWINDOW);

    // Create the window
    hMainWindow = CreateWindowEx(
        0,                          // Optional extended styles
        CLASS_NAME,                 // Window class
        "File Access Coordinator",   // Window title
        WS_OVERLAPPEDWINDOW,        // Window style
        // Position and size
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional app data
    );

    if (!hMainWindow) {
        MessageBox(NULL, "Failed to create window.", "Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
  }

  if (needToLoadDefault)
  {
      gCoordinator.init(static_filepath, static_user, static_sshHostname, static_sshUsername, static_sshPassword, static_remoteBaseDir);
      save_config();
  }
  else
  {
      gCoordinator.init(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
  }

  int result = 0;
  if (gOnlyCommandLine)
  {
    result = main_console(commandLine);
  }
  else
  {
    SetWindowText(hUserTextbox, gCoordinator.mUser.get_data());
    SetWindowText(hFileTextbox, gCoordinator.mFullLocalPath.get_data());

    // Main message loop
    using clock = std::chrono::steady_clock;
    auto nextReserveStatePoll = clock::now() + std::chrono::seconds(1);
    while (poll_win_message(false))
    {
      auto now = clock::now();
      if (now > nextReserveStatePoll)
      {
          load_config(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
          gCoordinator.mUser = user;
          gCoordinator.set_filepath(filepath);

          nextReserveStatePoll = now + std::chrono::seconds(1);
          s64 reservedFileSize;
          //TODO deal with file not existing
          reserveState = gCoordinator.get_reserve_state(&reservedFileRemoteOwner, &reservedFileSize);
          char lblReserveState[BSE_STACK_BUFFER_SMALL];
          if (gSimpleGui)
          {
            if (reserveState == ReserveState::UNKNOWN)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "Datenbank am Server nicht vorhanden.");
            }
            else if (reserveState == ReserveState::NOT_RESERVED)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "Datenbank ist zur bearbeitung freigegeben.");
            }
            else if (reserveState == ReserveState::RESERVED_BY_ME)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "Datenbank lokal in bearbeitung.");
            }
            else if (reserveState == ReserveState::RESERVED_BY_OTHER)
            {
              s64 ipOffset = reservedFileRemoteOwner.is_empty() ? 0 : string_find_last(reservedFileRemoteOwner.get_data(), ' ') - reservedFileRemoteOwner.get_data();
              string_format(lblReserveState, sizeof(lblReserveState), "Datenbank wird derzeit von '", reservedFileRemoteOwner.str.substr(0,ipOffset).c_str(), "' bearbeitet.");
            }
          }
          else
          {
            if (reserveState == ReserveState::UNKNOWN)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "File doesn't exist on the server.");
            }
            else if (reserveState == ReserveState::NOT_RESERVED)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "File not reserved.");
            }
            else if (reserveState == ReserveState::RESERVED_BY_ME)
            {
              string_format(lblReserveState, sizeof(lblReserveState), "File reserved by you.");
            }
            else if (reserveState == ReserveState::RESERVED_BY_OTHER)
            {
              s64 ipOffset = reservedFileRemoteOwner.is_empty() ? 0 : string_find_last(reservedFileRemoteOwner.get_data(), ' ') - reservedFileRemoteOwner.get_data();
              string_format(lblReserveState, sizeof(lblReserveState), "File reserved by '", reservedFileRemoteOwner.str.substr(0,ipOffset).c_str(), "'");
            }
          }

          SetWindowText(hReserveStateLabel, lblReserveState);
      }

      result = 0;
    }
  }

  gCoordinator.shutdown_session();
  return result;
}
