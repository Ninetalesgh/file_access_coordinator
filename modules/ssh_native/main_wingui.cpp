#define NO_GODOT
#define FAC_WINGUI
//#define POLL_WINDOWS_IN_SFTP_THREAD

#include "access_coordinator.cpp"

#include <windows.h>
#include <uxtheme.h>
#include "default_access.h"

#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comdlg32.lib")


//I made this part c++17, it was easier to get going right away
AccessCoordinator gCoordinator;
bool gRunning = true;

#undef log_info
#undef log_warning
#undef log_error
#define log_info( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_INFO, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_warning( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_WARNING, bse::debug::LogOutputType::ALL), __VA_ARGS__)
#define log_error( ... ) bse::debug::log(bse::debug::LogParameters(&gCoordinator, bse::debug::LogSeverity::BSE_LOG_SEVERITY_ERROR, bse::debug::LogOutputType::ALL), __VA_ARGS__)


namespace fs = std::filesystem;

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

void print_time()
{
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm = *std::localtime(&now_time);
  std::cout << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << '\n';
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

void print_current_context()
{
  log_info("Current user: ", gCoordinator.mUser.get_data(), "\n");
  log_info("Current file: ", gCoordinator.mFullLocalPath.get_data(), "\n");
}

int evaluate_expression(char const* expression)
{
  if (string_begins_with(expression, "show"))
  {
    print_current_context();
  }
  else if (string_begins_with(expression, "download"))
  {
    print_time();
    gCoordinator.download();
  }
  else if (string_begins_with(expression, "upload"))
  {
    print_time();
    gCoordinator.upload();
  }
  else if (string_begins_with(expression, "reserve"))
  {
    print_time();
    gCoordinator.reserve();
  }
  else if (string_begins_with(expression, "release"))
  {
    print_time();
    gCoordinator.release(false);
  }
  else if (string_begins_with(expression, "forcerelease"))
  {
    print_time();
    gCoordinator.release(true);
  }
  else if (string_begins_with(expression, "user"))
  {
    std::string input;
    if (string_begins_with(expression, "user="))
    {
      input = expression + 5;
      if (input.size() >= 2 && input.front() == '"' && input.back() == '"')
      {
        input = input.substr(1, input.size() - 2);
      }
    }
    else
    {
      std::cout << "Set current context user: ";
      std::getline(std::cin, input);
    }
    std::cout << "Setting user to: " << input << '\n';
    gCoordinator.mUser = input;
  }
  else if (string_begins_with(expression, "file"))
  {
    std::string input;
    if (string_begins_with(expression, "file="))
    {
      input = expression + 5;
      if (input.size() >= 2 && input.front() == '"' && input.back() == '"')
      {
        input = input.substr(1, input.size() - 2);
      }
    }
    else
    {
      std::cout << "Set current context file: ";
      std::getline(std::cin, input);
    }
    std::cout << "Setting file to: " << input << '\n';
    gCoordinator.set_filepath(input);
  }
  else if (string_begins_with(expression, "loadconfig"))
  {
    std::string filepath;
    std::string user;
    std::string sshHostname;
    std::string sshUsername;
    std::string sshPassword;
    std::string remoteBaseDir;
    if (load_config(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir))
    {
      gCoordinator.init(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);  
      std::cout << "Loaded access.config from disk.\n";
      print_current_context();
    }
  }
  else if (string_begins_with(expression, "saveconfig"))
  {
    if (save_config())
    {
      std::cout << "Saved cached configuration.\n";
      print_current_context();
    }
  }
  else if (string_begins_with(expression, "exit"))
  {
    return false;
  }
  else if (string_begins_with(expression, "agreeall"))
  {
    gCoordinator.mAgreeAllPrompts = true; 
  }
  else
  {
    std::cout << "Command unknown, available expressions are:\n"
             "----------------------------------------------\n"
             "--- Primary File Commands --------------------\n"
             "download -> Downloads the configured file from the server, overwriting your local version.\n"
             "upload -> Uploads your local version of the file to the server, overwriting the remote version.\n"
             "----------------------------------------------\n"
             "--- Secondary File Commands ------------------\n"
             "reserve -> Explicitly reserve the remote file for your current user and IP without otherwise manipulating files.\n"
             "release -> Explicitly release the remote file so other people can access it without otherwise manipulating files.\n"
             "forcerelease -> Force release the remote file, no matter who currently has it reserved.\n"
             "----------------------------------------------\n"
             "--- Local & Config Commands ------------------\n"
             "show -> Shows current context user and filepath.\n"
             "user -> Set the current context user (user=\"example\" to input the prompt directly).\n"
             "file -> Set the current context filepath (file=\"example filepath\" to input the prompt directly).\n"
             "loadconfig -> Reloads configuration from access.config, useful if you made manual changes to access.config.\n"
             "saveconfig -> Save the current configuration to access.config.\n"
             "exit -> Exits the application.\n"
             "agreeall -> Confirms all file related prompts without asking, place as first argument if gRunning from command line.\n";
  }
  return true;
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
    //ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

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

#define COLUMN_0_WIDTH 100 
#define COLUMN_1_WIDTH 40
#define COLUMN_2_WIDTH 420
#define COLUMN_3_WIDTH 100

#define ROW_0_HEIGHT 20
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

#define WINDOW_WIDTH  COLUMN_4_OFFSET
#define WINDOW_HEIGHT ROW_3_OFFSET

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

        char const* lblUser = "User: ";
        char const* lblFile = "File: ";
        RECT rectRow0 = { COLUMN_1_OFFSET, ROW_0_OFFSET, COLUMN_1_OFFSET + COLUMN_1_WIDTH, ROW_0_OFFSET + ROW_0_HEIGHT };
        RECT rectRow1 = { COLUMN_1_OFFSET, ROW_1_OFFSET, COLUMN_1_OFFSET + COLUMN_1_WIDTH, ROW_1_OFFSET + ROW_1_HEIGHT };
        
        DrawText(hdc, lblUser, -1, &rectRow0, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
        DrawText(hdc, lblFile, -1, &rectRow1, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
    
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
            SetWindowTheme(hButtonUpload, L"", L"");
            SetWindowTheme(hButtonDownload, L"", L"");
            SetWindowTheme(hButtonReserve, L"", L"");
            SetWindowTheme(hButtonRelease, L"", L"");
            SetWindowTheme(hButtonForceRelease, L"", L"");
            SetWindowTheme(hButtonSaveConfig, L"", L"");

            hTextBoxBrush = CreateSolidBrush(textBkColor);
            hButtonBrush = CreateSolidBrush(buttonBkColor);

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
                        gCoordinator.upload();
                        break;
                    }
                    case (int)HwndId::Button_Download:
                    {
                        gCoordinator.download();
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
    char const* log = gCoordinator.fetch_output().get_data();
    if (*log)
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

bool confirm_dialog_callback(char const* title, char const* message)
{
    int result = MessageBox(
        hMainWindow,
        message,
        title,
        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
    return result == IDYES;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int nCmdShow) 
{
    const char CLASS_NAME[] = "File Access Coordinator";
    AllocConsole();
    freopen("CONOUT$", "w", stdout);  // Redirect stdout
    freopen("CONIN$", "r", stdin);    // Redirect stdin
    freopen("CONOUT$", "w", stderr);  // Redirect stderr

    gCoordinator.mConfirmationDialogCallback = &confirm_dialog_callback;
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

    std::string filepath;
    std::string user;
    std::string sshHostname;
    std::string sshUsername;
    std::string sshPassword;
    std::string remoteBaseDir;
    bool needToLoadDefault = !load_config(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);

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

    if (needToLoadDefault)
    {
        gCoordinator.init(static_filepath, static_user, static_sshHostname, static_sshUsername, static_sshPassword, static_remoteBaseDir);
        save_config();
    }
    else
    {
        gCoordinator.init(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
    }

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
            nextReserveStatePoll += std::chrono::seconds(1);
            s64 reservedFileSize;
            //TODO deal with file not existing
            reserveState = gCoordinator.get_reserve_state(&reservedFileRemoteOwner, &reservedFileSize);
            char lblReserveState[BSE_STACK_BUFFER_SMALL];
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

            SetWindowText(hReserveStateLabel, lblReserveState);
        }
    }

    gCoordinator.shutdown_session();
    return 0;
}
