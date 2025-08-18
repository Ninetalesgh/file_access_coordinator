#define NO_GODOT

#include "access_coordinator.cpp"

#include "default_access.h"

#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

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
  else if (string_begins_with(expression, "rollback"))
  {
    std::string input;
    {
      std::cout << "How many versions back do you want to go? (0 to 4): ";
      std::getline(std::cin, input);
    }
    if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit))
    {
      int stepsBack = std::clamp(std::stoi(input), 0, 4);
      coordinator.rollback(stepsBack);
    }
    else
    {
      log_info("Aborting rollback.");
    }
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
             "rollback -> Overwrites the main file with one of the available backup files.\n"
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

int loop_wait_for_expression()
{
  while(running)
  {
    std::string input;
    std::cout << "----------------------------------------------\nCommand: ";
    std::getline(std::cin, input);
    if (!evaluate_expression(input.c_str())) running = false;
  }

  return 0;
}

int main(int argc, char** argv)
{
  load_config();

  for (int i = 1; i < argc; ++i)
  {
    if (!evaluate_expression(argv[i])) running = false;
  }

  loop_wait_for_expression();

  coordinator.shutdown_session();

  return 0;
}
