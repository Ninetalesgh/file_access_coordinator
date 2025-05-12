#define NO_GODOT

#include "access_coordinator.cpp"

#include "default_access.h"

#include <filesystem>
#include <cstdlib>
#include <fstream>

//I made this part c++, it was easier to get going right away
AccessCoordinator coordinator;
bool running = true;

namespace fs = std::filesystem;

bool save_config()
{
  char const* appData = std::getenv("APPDATA");
  if (!appData)
  {
    std::cerr << "APPDATA environment variable not found.\n";
    return false;
  }

  if (!coordinator.is_current_base_config_valid())
  {
    std::cout << "- Error: current config is not valid, not writing to access.config.";
    return false;
  }

  fs::path targetPath = fs::path(appData) / "Godot" / "app_userdata" / "file_access_coordinator";
  fs::path filePath = targetPath / "access.config";

  if (!fs::exists(targetPath))
  {
    std::error_code ec;
    if (fs::create_directories(targetPath, ec))
    {
      std::cout << "Created directory: " << targetPath << '\n';
    }
    else
    {
      std::cerr << "Failed to create directory: " << ec.message() << '\n';
      return false;
    }
  }

  std::ofstream configFileWriteStream(filePath);
  if (configFileWriteStream)
  {
    configFileWriteStream << "filepath=" << coordinator.mFullLocalPath.str;
    configFileWriteStream << "user=" << coordinator.mUser.str;
    configFileWriteStream << "sshHostname=" << coordinator.mSshHostname.str;
    configFileWriteStream << "sshUsername=" << coordinator.mSshUsername.str;
    configFileWriteStream << "sshPassword=" << coordinator.mSshPassword.str;
    configFileWriteStream << "remoteBaseDir=" << coordinator.mRemoteBaseDir.str;
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
    return false;
  }

  fs::path targetPath = fs::path(appData) / "Godot" / "app_userdata" / "file_access_coordinator";
  fs::path filePath = targetPath / "access.config";

  if (!fs::exists(targetPath))
  {
    std::error_code ec;
    if (fs::create_directories(targetPath, ec))
    {
      std::cout << "Created directory: " << targetPath << '\n';
    }
    else
    {
      std::cerr << "Failed to create directory: " << ec.message() << '\n';
      return false;
    }
  }

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
    coordinator.init(static_filepath, static_user, static_sshHostname, static_sshUsername, static_sshPassword, static_remoteBaseDir);
    save_config();
  }

  return true;
}

int evaluate_expression(char const* expression)
{
  if (string_begins_with(expression, "download"))
  {
    coordinator.download();
  }
  else if (string_begins_with(expression, "upload"))
  {
    coordinator.upload();
  }
  else if (string_begins_with(expression, "reserve"))
  {
    coordinator.reserve();
  }
  else if (string_begins_with(expression, "release"))
  {
    coordinator.release(false);
  }
  else if (string_begins_with(expression, "forcerelease"))
  {
    coordinator.release(true);
  }
  else if (string_begins_with(expression, "init"))
  {
    load_config();
  }
  else if (string_begins_with(expression, "exit"))
  {
    return false;
  }
  else
  {
    std::cout << "Command unknown, available expressions are:\n"
             "init -> Reloads configuration.\n"
             "exit -> Exits the application.\n"
             "download -> Downloads the configured file from the server, overwriting your local version.\n"
             "upload -> Uploads your local version to the server, overwriting the remote version.\n"
             "reserve -> Explicitly reserve the remote file for your current user and IP without otherwise manipulating files.\n"
             "release -> Explicitly release the remote file so other people can access it without otherwise manipulating files.\n"
             "forcerelease -> Explicitly FORCE the release of the remote file, no matter who currently has it reserved, anyone can access it after.\n";
  }
  return true;
}

int loop_wait_for_expression()
{
  while(running)
  {
    std::string input;
    std::cout << "Command: ";
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
