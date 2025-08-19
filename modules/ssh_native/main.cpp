#define NO_GODOT

#include "access_coordinator.cpp"
#include "cli_parser.h"

#include "default_access.h"


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

bool load_config(std::string& filepath, std::string& user, std::string& sshHostname, std::string& sshUsername, std::string& sshPassword, std::string& remoteBaseDir)
{
  log_info("full empty run on linux not tested, please make sure the config looks clean.");
  return load_config();
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

int loop_wait_for_expression()
{
  while(running)
  {
    std::string input;
    std::cout << "----------------------------------------------\nCommand: ";
    std::getline(std::cin, input);
    if (!evaluate_expression(input.c_str(), coordinator)) running = false;
  }

  return 0;
}

int main(int argc, char** argv)
{
  load_config();

  for (int i = 1; i < argc; ++i)
  {
    if (!evaluate_expression(argv[i], coordinator)) running = false;
  }

  loop_wait_for_expression();

  coordinator.shutdown_session();

  return 0;
}
