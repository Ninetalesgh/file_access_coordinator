#define NO_GODOT
#define FAC_WINGUI


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


//I made this part c++17, it was easier to get going right away
AccessCoordinator coordinator;
bool running = true;

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

  if (!coordinator.is_current_base_config_valid())
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
    configFileWriteStream << "filepath=" << coordinator.mFullLocalPath.str << '\n';
    configFileWriteStream << "user=" << coordinator.mUser.str << '\n';
    configFileWriteStream << "sshHostname=" << coordinator.mSshHostname.str << '\n';
    configFileWriteStream << "sshUsername=" << coordinator.mSshUsername.str << '\n';
    configFileWriteStream << "sshPassword=" << coordinator.mSshPassword.str << '\n';
    configFileWriteStream << "remoteBaseDir=" << coordinator.mRemoteBaseDir.str << '\n';
    return true;
  }

  return false;
}

bool load_config()
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

    std::string filepath;
    std::string user;
    std::string sshHostname;
    std::string sshUsername;
    std::string sshPassword;
    std::string remoteBaseDir;

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

    coordinator.init(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
  }
  else
  {
    //use default values and save to config
    coordinator.init(static_filepath, static_user, static_sshHostname, static_sshUsername, static_sshPassword, static_remoteBaseDir);
    save_config();
  }

  return true;
}

void print_current_context()
{
  std::cout << "Current user: " << coordinator.mUser.str << '\n';
  std::cout << "Current file: " << coordinator.mFullLocalPath.str << '\n';
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
    coordinator.download();
  }
  else if (string_begins_with(expression, "upload"))
  {
    print_time();
    coordinator.upload();
  }
  else if (string_begins_with(expression, "reserve"))
  {
    print_time();
    coordinator.reserve();
  }
  else if (string_begins_with(expression, "release"))
  {
    print_time();
    coordinator.release(false);
  }
  else if (string_begins_with(expression, "forcerelease"))
  {
    print_time();
    coordinator.release(true);
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
    coordinator.mUser = input;
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
    coordinator.set_filepath(input);
  }
  else if (string_begins_with(expression, "loadconfig"))
  {
    if (load_config())
    {
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
    coordinator.mAgreeAllPrompts = true; 
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
             "agreeall -> Confirms all file related prompts without asking, place as first argument if running from command line.\n";
  }
  return true;
}

HWND hButtonUpload;
HWND hButtonDownload;
HWND hButtonReserve;
HWND hButtonRelease;
HWND hButtonForceRelease;
HWND hButtonSaveConfig;

enum class ButtonId : int
{
    BUTTON_MIN = 0,
    Button_Upload,
    Button_Download,
    Button_Reserve,
    Button_Release,
    Button_ForceRelease,
    Button_SaveConfig,
    BUTTON_MAX
};

char const* get_button_text(int buttonId)
{
    switch(buttonId)
    {
        case ButtonId::Button_Upload: return "Upload";
        case ButtonId::Button_Download: return "Download";
        case ButtonId::Button_Reserve: return "Reserve";
        case ButtonId::Button_Release: return "Release";
        case ButtonId::Button_ForceRelease: return "Force Release";
        case ButtonId::Button_SaveConfig: return "Save Config";
    }
    return "";
}

HWND hMainWindow;
HWND hUserTextbox;
HWND hFileTextbox;
HWND hOutputTextbox;

#define BUTTON_HEIGHT 20
#define BUTTON_WIDTH 100
#define TEXTBOX_HEIGHT 20
#define TEXTBOX_WIDTH 200
#define UI_PADDING 2

HBRUSH hTextBoxBrush;
HBRUSH hButtonBrush;
HBRUSH hWindowBrush;
COLORREF textBkColor = RGB(30,30,30);
COLORREF textForegroundColor = RGB(200,200,200);
COLORREF buttonBkColor = RGB(75, 75, 75);
COLORREF buttonForegroundColor = RGB(255, 255, 255);
COLORREF windowBkColor = RGB(50, 50, 50);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

            hUserTextbox = CreateWindowEx(
                0, "EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                BUTTON_WIDTH + UI_PADDING * 2, TEXTBOX_HEIGHT * 0 + UI_PADDING * 1, TEXTBOX_WIDTH, TEXTBOX_HEIGHT,  // x, y, width, height
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hFileTextbox = CreateWindowEx(
                0, "EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                BUTTON_WIDTH + UI_PADDING * 2, TEXTBOX_HEIGHT * 1 + UI_PADDING * 2, TEXTBOX_WIDTH, TEXTBOX_HEIGHT,  // x, y, width, height
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonUpload = CreateWindow(
                "BUTTON", "Upload",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 1 + BUTTON_HEIGHT * 0, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_Upload, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonDownload = CreateWindow(
                "BUTTON", "Download",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 2 + BUTTON_HEIGHT * 1, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_Download, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonReserve = CreateWindow(
                "BUTTON", "Reserve",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 3 + BUTTON_HEIGHT * 2, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_Reserve, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonRelease = CreateWindow(
                "BUTTON", "Release",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 4 + BUTTON_HEIGHT * 3, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_Release, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonForceRelease = CreateWindow(
                "BUTTON", "Force Release",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 5 + BUTTON_HEIGHT * 4, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_ForceRelease, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            hButtonSaveConfig = CreateWindow(
                "BUTTON", "Save Config",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                UI_PADDING * 1, UI_PADDING * 6 + BUTTON_HEIGHT * 5, BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)ButtonId::Button_SaveConfig, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            return 0;
        }
        case WM_DESTROY:
            DeleteObject(hTextBoxBrush);
            DeleteObject(hWindowBrush);
            DeleteObject(hButtonBrush);
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: 
        {
            return 0;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis->CtlID > (int)ButtonId::BUTTON_MIN && dis->CtlID < (int)ButtonId::BUTTON_MAX) {
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
        case WM_COMMAND:
        {
            char buffer[BSE_STACK_BUFFER_SMALL];
            buffer[0] = '\0';
            GetWindowText(hUserTextbox, buffer, sizeof(buffer));
            if (buffer[0]) coordinator.mUser = buffer;
            buffer[0] = '\0';
            GetWindowText(hFileTextbox, buffer, sizeof(buffer));
            if (buffer[0]) coordinator.set_filepath(String(buffer));
            
            switch (LOWORD(wParam))
            {
                case (int)ButtonId::Button_Upload:
                {
                    printf("upload clicked");
                    coordinator.upload();
                    break;
                }
                case (int)ButtonId::Button_Download:
                {
                    printf("download clicked");
                    coordinator.download();
                    break;
                }
                case (int)ButtonId::Button_Reserve:
                {
                    printf("reserve clicked");
                    coordinator.reserve();
                    break;
                }
                case (int)ButtonId::Button_Release:
                {
                    printf("release clicked");
                    coordinator.release(false);
                    break;
                }
                case (int)ButtonId::Button_ForceRelease:
                {
                    printf("release clicked");
                    coordinator.release(true);
                    break;
                }
                case (int)ButtonId::Button_SaveConfig:
                {
                    save_config();
                    MessageBox(hwnd, ".", "Config Saved!", MB_OK);
                    break;
                }
                default:
                {}
            }
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
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

bool poll_win_message()
{
    MSG msg = {};
    if (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return true;
    }

    return false;
}

bool confirm_dialog_callback(char const* title, char const* message)
{
    if (title && message)
    {
        SetForegroundWindow(hMainWindow);
        int result = MessageBox(
            NULL,
            message,
            title,
            MB_YESNO | MB_ICONQUESTION | MB_TOPMOST
        );
        return result == IDYES;
    }

    return false;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int nCmdShow) 
{
    const char CLASS_NAME[] = "File Access Coordinator";
    AllocConsole();
    freopen("CONOUT$", "w", stdout);  // Redirect stdout
    freopen("CONIN$", "r", stdin);    // Redirect stdin
    freopen("CONOUT$", "w", stderr);  // Redirect stderr

    load_config();
    coordinator.mConfirmationDialogCallback = &confirm_dialog_callback;
    hWindowBrush = CreateSolidBrush(windowBkColor);
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hWindowBrush;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Create the window
    hMainWindow = CreateWindowEx(
        0,                          // Optional extended styles
        CLASS_NAME,                 // Window class
        "File Access Coordinator",   // Window title
        WS_OVERLAPPEDWINDOW,        // Window style
        // Position and size
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
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

    // Main message loop
    while (poll_win_message()) {}

    coordinator.shutdown_session();
    return 0;
}
